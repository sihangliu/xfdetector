#
# Copyright 2019, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
"""Set of classes that represent the context of single test execution
(like build, filesystem, test type)"""

import os
import sys
import itertools
import subprocess as sp

import helpers as hlp

try:
    import envconfig
    envconfig = envconfig.config
except ImportError:
    # if file doesn't exist create dummy object
    envconfig = {'GLOBAL_LIB_PATH': ''}


def expand(*classes):
    """Return flatten list of container classes with removed duplications"""
    return list(set(itertools.chain(*classes)))


class Context:
    """Manage test execution based on values from context classes"""

    def __init__(self, test, conf, fs, build):
        self.env = {}
        for ctx in [fs, build]:
            if hasattr(ctx, 'env'):
                self.env.update(ctx.env)
        self.test = test
        self.conf = conf
        self.build = build
        self.msg = hlp.Message(conf)
        self.fs = fs

    @property
    def testdir(self):
        """Test directory on selected filesystem"""
        # Testdir uses 'fs.dir' field  - it is illegal to access it in case of
        # 'Non' fs. Hence it is implemented as a property.
        return os.path.join(self.fs.dir, self.test.testdir)

    def create_holey_file(self, size, name):
        """Create a new file with the selected size and name"""
        filepath = os.path.join(self.testdir, name)
        with open(filepath, 'w') as f:
            f.seek(size)
            f.write('\0')
        return filepath

    def exec(self, cmd, args='', expected_exit=0):
        """Execute binary in current test context"""
        env = {**self.env, **os.environ.copy(), **self.test.utenv}
        if sys.platform == 'win32':
            env['PATH'] = self.build.libdir + os.pathsep +\
                          envconfig['GLOBAL_LIB_PATH'] + os.pathsep +\
                          env.get('PATH', '')
            cmd = os.path.join(self.build.exedir, cmd) + '.exe'
        else:
            env['LD_LIBRARY_PATH'] = self.build.libdir + os.pathsep +\
                                     envconfig['GLOBAL_LIB_PATH'] +\
                                     os.pathsep +\
                                     env.get('LD_LIBRARY_PATH', '')
            cmd = os.path.join(self.test.cwd, cmd) + self.build.exesuffix

        proc = sp.run([cmd, args], env=env, cwd=self.test.cwd,
                      timeout=self.conf.timeout, stdout=sp.PIPE,
                      stderr=sp.STDOUT, universal_newlines=True)

        if proc.returncode != expected_exit:
            self.test.fail(proc.stdout)
        else:
            self.msg.print_verbose(proc.stdout)


class _CtxType(type):
    """Metaclass for context classes that can stand for multiple classes"""
    def __init__(cls, name, bases, dct):
        type.__init__(cls, name, bases, dct)

        # it is supposed that context class has a user defined base class
        if cls.__base__.__name__ == 'object':
            return

        # context class preferred to be used when 'Any' test option is provided
        if not hasattr(cls, 'preferred'):
            cls.is_preferred = False

        # if explicit, run only if the test case explicitly specifies this
        # context in its own configuration
        if not hasattr(cls, 'explicit'):
            cls.explicit = False

        if hasattr(cls, 'includes'):
            cls.includes = expand(*cls.includes)
        else:
            # class that does not include other classes, includes itself
            cls.includes = [cls]

        # set instance (not class) method
        setattr(cls, '__repr__', lambda cls: cls.__class__.__name__.lower())

    def __repr__(cls):
        return cls.__name__.lower()

    def __iter__(cls):
        for c in cls.includes:
            yield c

    def factory(cls, conf, *classes):
        return [c(conf) for c in expand(*classes)]


class _Build(metaclass=_CtxType):
    """Base and factory class for standard build classes"""
    exesuffix = ''


class Debug(_Build):
    """Set the context for debug build"""
    is_preferred = True

    def __init__(self, conf):
        if sys.platform == 'win32':
            self.exedir = hlp.WIN_DEBUG_EXEDIR
        self.libdir = hlp.DEBUG_LIBDIR


class Nondebug(_Build):
    """Set the context for nondebug build"""
    is_preferred = True

    def __init__(self, conf):
        if sys.platform == 'win32':
            self.exedir = hlp.WIN_NONDEBUG_EXEDIR
        self.libdir = hlp.NONDEBUG_LIBDIR


# Build types not available on Windows
if sys.platform != 'win32':
    class Static_Debug(_Build):
        """Sets the context for static_debug build"""

        def __init__(self, conf):
            self.exesuffix = '.static-debug'
            self.libdir = hlp.DEBUG_LIBDIR

    class Static_Nondebug(_Build):
        """Sets the context for static_nondebug build"""

        def __init__(self, conf):
            self.exesuffix = '.static-nondebug'
            self.libdir = hlp.NONDEBUG_LIBDIR


class _Fs(metaclass=_CtxType):
    """Base class for filesystem classes"""


class Pmem(_Fs):
    """Set the context for pmem filesystem"""
    is_preferred = True

    def __init__(self, conf):
        self.dir = os.path.abspath(conf.pmem_fs_dir)
        if conf.fs_dir_force_pmem == 1:
            self.env = {'PMEM_IS_PMEM_FORCE': '1'}


class Nonpmem(_Fs):
    """Set the context for nonpmem filesystem"""
    def __init__(self, conf):
        self.dir = os.path.abspath(conf.non_pmem_fs_dir)


class Non(_Fs):
    """
    No filesystem is used. Accessing some fields of this class is prohibited.
    """
    explicit = True

    def __init__(self, conf):
        pass

    def __getattribute__(self, name):
        if name in ('dir',):
            raise AttributeError("fs '{}' attribute cannot be used for '{}' fs"
                                 .format(name, self))
        else:
            return super().__getattribute__(name)


class _TestType(metaclass=_CtxType):
    """Base class for test duration"""


class Short(_TestType):
    pass


class Medium(_TestType):
    pass


class Long(_TestType):
    pass


class Check(_TestType):
    includes = [Short, Medium]
