#include "xfdetector.hh"
#include <sys/time.h>

#include <regex>

string ExeCtrl::rename_pool_img(string new_pool_name) 
{
    string result("");

    for (auto cmd_param : target_cmd) {
        if (cmd_param.find(POOL_IMAGE_IDENTIFIER) != string::npos) {
            result += new_pool_name;
        } else {
            result += cmd_param;
        }
        result += " ";
    }

    return string(result.begin(), result.end()-1);
}

void ExeCtrl::init(int _exec_id, std::vector<string> args)
{
    // Set execution id
    exec_id = _exec_id;

    // Add execution id to the pintool options
    if (exec_id >= 0) {
        pin_pre_failure_option += PIN_SET_EXECID(exec_id);
        pin_post_failure_option += PIN_SET_EXECID(exec_id);
    }
    if (!failure_point_file.empty()) {
        pin_pre_failure_option += PIN_SET_FAILURE_FILE(failure_point_file);
    }

    // Parse commands according to config file    
    parse_exec_command(args);
}

char *ExeCtrl::change_env(char *kv) {
    char *xfd_preload = "XFD_LD_PRELOAD";

    char *result = kv;    
    if (!strncmp(xfd_preload, kv, sizeof(xfd_preload))) {
        result += 4;
    }
    return result;
}

void ExeCtrl::execute_pre_failure()
{
    char** pre_failure_command = genPinCommand(PRE_FAILURE, pm_image_name);

    int cpid = fork();
    if (cpid < 0) {
        ERR("Fork failed.");
    }
    if (!cpid) {
        // Child
        // Prepare env
        char *env[1024];
        int idx = 0;
        for(char **current = environ; *current; current++) {
            env[idx++] = change_env(*current);
        }
        env[idx++] = NULL;
        // env[0] = alloc_print("PMEM_MMAP_HINT=%llx", PM_ADDR_BASE);
        // env[1] = NULL;

        if (execvpe(pre_failure_command[0], pre_failure_command, env) < 0) {
            ERR("Execution of pre-failure command failed.");
        }
        // if (system(cmd2str(pre_failure_command).c_str()) < 0) {
        //     ERR("Execution of pre-failure command failed.");
        // }
        // Terminate child process
        // exit(0);
    } else {
        // Parent
        int victim = 0;
        while(pre_failure_command[victim]) {
            free(pre_failure_command[victim++]);
        }
        free(pre_failure_command);
        pre_failure_pid = cpid;
    }
}

string ExeCtrl::execute_post_failure()
{   
    string image_copy_name = copy_pm_image();

    // Execute recovery code on the PM image copy
    // string image_copy_name = copy_name_queue.front();
    char** post_failure_command = genPinCommand(POST_FAILURE, image_copy_name); // + string(" 2>> post.out");

    int cpid = fork();
    if (cpid < 0) {
        ERR("Fork failed.");
    }
    if (!cpid) {
        // Child
        // Prepare env
        char *env[1024];
        int idx = 0;
        for(char **current = environ; *current; current++) {
            env[idx++] = change_env(*current);
        }
        env[idx++] = alloc_print("POST_FAILURE=1");
        env[idx++] = NULL;
        // env[0] = alloc_print("POST_FAILURE=1");
        // env[1] = alloc_print("PMEM_MMAP_HINT=%llx", PM_ADDR_BASE);
        // env[2] = NULL;
        
        if (execvpe(post_failure_command[0], post_failure_command, env) < 0) {
            ERR("Execution of post-failure command failed.");
        }
        // if (system(cmd2str(post_failure_command).c_str()) < 0) {
        //     ERR("Execution of post-failure command failed.");
        // }
        // Terminate child process
        // exit(0);
    } else {
        // Parent
        int victim = 0;
        while(post_failure_command[victim]) {
            free(post_failure_command[victim++]);
        }
        free(post_failure_command);
        post_failure_pid = cpid;
        return image_copy_name;
    }
}

string ExeCtrl::copy_pm_image()
{
    // Use tid to name copy image
    srand(time(NULL));
    string copy_name = pm_image_name + "_xfdetector_" + std::to_string(rand());
    
    string copy_command = "cp " + pm_image_name + " " + copy_name;
    
    if (system(copy_command.c_str()) < 0)
        ERR("Cannot copy image: " + pm_image_name);

    return copy_name;
}

/**
 * Read and return the env_variable as a std::string
 */
string get_env_var(string name)
{
    string result;

    char *env_val = std::getenv(name.c_str());

    if (env_val != nullptr) {
        result = std::string(env_val);
    } else {
        result = "";    
    }

    return result;
}

void ExeCtrl::parse_exec_command(std::vector<string> args)
{
    size_t arg_iter = 0;
    string option("");
    string loc_pool_image_name("");

    auto err_and_exit = [](string msg) {
        std::cout << "ERROR: " << msg << std::endl;
        std::cout << std::endl << HELP_STR << std::endl;
        exit(1);    
    };

    if (args.size()-1 < 3) {
        err_and_exit("Required arguments missing.");    
    }

    if (std::find(args.begin(), args.end(), "--") != args.end()) { 
        size_t idx = std::find(args.begin(), args.end(), "--") - args.begin();

        if (idx - 1 < 2) {
            err_and_exit("Required arguments missing.");
        }
    }
    
    for (auto arg : args) {
        if (arg_iter == 1) {
            pintool_path = arg;
        } else if (arg_iter == 2) {
            loc_pool_image_name = arg;
        } else {
            option = "--failure-points=";
            if (arg.substr(0, option.size()) == option) {    
                failure_point_file = string(arg.begin()+option.size(), arg.end());
            }

            option = "--";
            if (arg == option) {
                if (arg_iter+1 >= args.size()) {
                    err_and_exit("No command target supplied.");
                } else {
                    target_cmd = std::vector<string>(args.begin()+arg_iter+1, args.end());
                }
                break;
            }
        }

        arg_iter++;
    }

    for (auto cmd_param : target_cmd) {
        if (cmd_param.find(POOL_IMAGE_IDENTIFIER) != string::npos) {
            pm_image_name = std::regex_replace(
                cmd_param, std::regex(POOL_IMAGE_IDENTIFIER), loc_pool_image_name);
        }    
    }

    std::cout << "---------Command line arguments---------" << endl;
    std::cout << "       pintool_path: " << pintool_path << std::endl;
    std::cout << "    pool_image_name: " << loc_pool_image_name << std::endl;
    std::cout << "            Command: ";
    for (auto cmd_param : target_cmd) std::cout << cmd_param << " ";
    std::cout << std::endl;
    std::cout << "Failure points file: " << failure_point_file << std::endl;
    std::cout << "      pm_image_name: " << pm_image_name << std::endl;
    std::cout << std::endl;

    pre_failure_exec_command = rename_pool_img(pm_image_name);
}

int ExeCtrl::post_failure_status()
{
    int status;
    if (waitpid(post_failure_pid, &status, 0) == -1 ) {
        perror("waitpid failed");
        return -1;
    }
    if (WIFEXITED(status)) {
        const int es = WEXITSTATUS(status);
        if (es < 0)
            return -1;
    }

    return 0;
}

void ExeCtrl::term_pre_failure()
{
    if (kill(pre_failure_pid, 9) < 0)
        ERR("Cannot kill process: " + std::to_string(pre_failure_pid));
}

void ExeCtrl::term_post_failure()
{
    if (kill(post_failure_pid, 9) < 0)
        ERR("Cannot kill process: " + std::to_string(post_failure_pid));
}

// void ExeCtrl::kill_proc(unsigned pid)
// {
//     if (kill(pid, 9) < 0) ERR("Cannot kill process: " + std::to_string(pid));
// }

char** ExeCtrl::genPinCommand(int stage, string pm_image_name)
{
    const char *pin_root = std::getenv("PIN_ROOT");
    if (strcmp(pin_root, "") != 0) {
        if (stage == PRE_FAILURE) {
            string str = string(pin_root) + "/pin -t " + pintool_path + " " + pin_pre_failure_option 
                    + " -- " +  pre_failure_exec_command;
            return str2cmd(str);
        } else if (stage == POST_FAILURE) {
            string str = string(pin_root) + "/pin -t " 
                    + pintool_path + " " + pin_post_failure_option 
                    + " -- " + rename_pool_img(pm_image_name);
            return str2cmd(str);
        }
    }
    else {
        XFD_ASSERT(0 && "Environment PIN_ROOT not set.");
    }
    return (char**)NULL;
}
