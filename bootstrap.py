#!/usr/bin/env python3

import subprocess
import sys
import os

def run(command_seq_, shell_=False, die_=False, output_=False, cwd_=None):

    if shell_:
        command_seq_=' '.join(command_seq_)

    proc = subprocess.run(command_seq_,
                          stderr=subprocess.STDOUT,
                          stdout=subprocess.PIPE,
                          shell=shell_,
                          cwd=cwd_,
                          )

    if die_ and proc.returncode:
        if not shell_:
            command_seq_=' '.join(command_seq_)
                    print('{0}\n\nfailed: {1}'.format(proc.stdout, command_seq_),
              file=sys.stderr,
              )
        exit(proc.returncode)

    if output_:
        return proc.stdout
    else:
        return proc.returncode


def comp_ver(exist_, required_, name_):

    if exist_ >= required_:
        return 0
    else:
        print("'{0}'version '{1}' does not meet required '{2}'".format(name_, exist_, required_),
              file=sys.stderr,
              )
        return 1


def run_pkg(sudo_, mgr_, opt_, pkg_):

    if sys.platform() == 'win32':
        return 1

    for i in range(len(mgr_)):
        if not run(['which', mgr_[i]]):
            return run([sudo_[i], mgr_[i], opt_[i], pkg_[i]], shell_=True)

    print('failed to find a package manager and install {0}'.format(pkg_[0]),
          file=sys.stderr,
          )
    return 1


def check_ninja():

    if run(['which', 'ninja']):

        SUDO = ["sudo",   "sudo",       "sudo",    "sudo",   "sudo",    "",        "sudo" ]
        MGR = [ "pacman", "apt-get",    "dnf",     "emerge", "port",    "brew",    "pkg" ]
        OPT = [ "-S",     "-y install", "install", "",       "install", "install", "install" ]
        PKG = [ "ninja",
                "ninja-build",
                "ninja-build",
                "dev-util/ninja",
                "ninja",
                "ninja",
                "ninja" ]
        run_pkg(SUDO, MGR, OPT, PKG)

    version_ = '1.7.2'

    if run(['which', 'ninja']) or comp_ver(run(['ninja', '--version'], output_=True), version_, 'ninja'):

        print('will try to download ninja binary...')

        url_ = "https://github.com/ninja-build/ninja/releases"

        if sys.platform.startswith('linux'):
            url_ = url_ + '/download/v{0}/ninja-linux.zip'.format(version_)
        elif sys.platform.startswith('darwin'):
            url_ = url_ + '/download/v{0}/ninja-mac.zip'.format(version_)
        else:
            print("don't know how to handle '{0}'".format(sys.platform))
            exit(1)

        run(['mkdir', '-p', './toolchain'], die_=True)
        run(['wget', '-O', './toolchain/ninja.zip', url_], die_=True)
        install_ = '/usr/local/bin'
        run(['sudo', 'unzip', '-o', '-d', install_ + '/', './toolchain/ninja.zip'], die_=True)
        run(['sudo', 'chmod', 'go+rx', install_ + '/ninja'], die_=True)

        if comp_ver(run_output(['ninja', '--version']), version_, 'ninja'):
            print("installed a binary ninja v{0} to {1}; 'ninja --version' still fails.\n\
                  Please check $PATH".format(version_, install_),
                  file=sys.stderr,
                  )
            exit(1)


def check_meson():

    if run(['which', 'meson']):

        if run(['which', 'pip3']):

            SUDO = ["sudo",        "sudo",     "" ]
            MGR = [ "apt-get",     "port",     "brew" ]
            OPT = [ "-y install",  "install",  "install" ]
            PKG = [ "python3-pip", "py35-pip", "python3" ]

            if run_pkg(SUDO, MGR, OPT, PKG):
                exit(1)

        run(['sudo', '-H', run(['which', 'pip3'], output_=True), 'install', 'meson'], die_=True)



def main():

    if not run(['which', 'cscope']):
        run(['cscope', '-b', '-q', '-U', '-I.include', '-s.src', '-s.test'], die_=True)

    check_ninja()

    check_meson()

    run(['rm', '-rfv', 'build*'], die_=True)

    BUILD_NAMES = [ "debug", "debug-opt",      "release", "plain", "tsan",                "asan" ]
    BUILD_TYPES = [ "debug", "debugoptimized", "release", "plain", "debugoptimized",      "debugoptimized" ]
    BUILD_OPTS = [  "",      "",               "",        "",      "-Db_sanitize=thread", "-Db_sanitize=address" ]
    BUILD_GRIND = [ "",      "yes",            "",        "",      "",                    "" ]
    BUILD_TRAVIS = ["",      "yes",            "yes",     "yes",   "",                    "" ]

    for i in range(len(BUILD_NAMES)):

        if run(['$TRAVIS'], shell_=True) and BUILD_TRAVIS[i]:
            continue

        run(['meson', BUILD_OPTS[i],
             '--buildtype', BUILD_TYPES[i],
             'build-{0}'.format(BUILD_NAMES[i]),
             ], shell_=True)

        run_dir_ = os.path.join(os.getcwd(), 'build-{0}'.format(BUILD_NAMES[i]))

        if BUILD_GRIND[i] and not run(['which', 'valgrind']):
            #shuld run and die? I suppose no.
            run(['VALGRIND=1', 'mesontest', "--wrap=\'valgrind --leak-check=full\'"],
                shell_=True,
                cwd_=run_dir_,
                )


main()


# TODO : add comments
# TODO : add Windows support
