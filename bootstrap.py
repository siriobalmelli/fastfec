#!/usr/bin/env python3

import subprocess
import platform
import sys
import os


def run_die(command_seq_, shell_=False):

    print('\n\nEXEC: %s' % ' '.join(command_seq_))

    if shell_ == True:
        proc = subprocess.Popen(' '.join(command_seq_), shell=True)
    else:
        proc = subprocess.Popen(command_seq_)
        
    proc.wait()
    
    if proc.returncode:
        print('failed: %s' % ' '.join(command_seq_), file=sys.stderr)
        exit(1)
    else:
        return 0


def run_return(command_seq_, shell_=False):

    if shell_ == True:
        proc = subprocess.Popen(' '.join(command_seq_), shell=True)
    else:
        proc = subprocess.Popen(command_seq_)
        
    proc.wait()
    
    return proc.returncode


def run_output(command_seq_):

    proc = subprocess.Popen(command_seq_,
                            stderr=subprocess.STDOUT,
                            stdout=subprocess.PIPE,
                            )
    stdout_, stderr_ = proc.communicate()
    return stdout_.decode('utf-8').strip()


def pushd(desired_dir_):
    current_ = os.getcwd()
    os.chdir(desired_dir_)
    return current_


def comp_ver(exist_, required_, name_):
    
    if exist_ >= required_:
        return 0
    else:
        print("'%s'version '%s' does not meet required '%s'" % (name_, exist_, required_), file=sys.stderr)
        return 1


def run_pkg(sudo_, mgr_, opt_, pkg_):

    if platform.system() == 'win32':
        return 1

    for i in range(len(mgr_)):
        if not run_return(['which', mgr_[i]]):
            return run_return([sudo_[i], mgr_[i], opt_[i], pkg_[i]], shell_=True)

    print('failed to find a package manager and install %s' % pkg_[0], file=sys.stderr)
    return 1


def check_ninja():

    if run_return(['which', 'ninja']):

        sudo_ = ["sudo",   "sudo",       "sudo",    "sudo",   "sudo",    "",        "sudo" ]
        mgr_ = [ "pacman", "apt-get",    "dnf",     "emerge", "port",    "brew",    "pkg" ]
        opt_ = [ "-S",     "-y install", "install", "",       "install", "install", "install" ]
        pkg_ = [ "ninja",
                "ninja-build",
                "ninja-build",
                "dev-util/ninja",
                "ninja",
                "ninja",
                "ninja" ]
        run_pkg(sudo_, mgr_, opt_, pkg_)

    version_ = '1.7.2'

    if run_return(['which', 'ninja']) or comp_ver(run_output(['ninja', '--version']), version_, 'ninja'):

        print('will try to download ninja binary...')

        url_ = "https://github.com/ninja-build/ninja/releases"
        platform_ = platform.system().lower()

        if platform_ == 'linux':
            url_ = url_ + '/download/v%s/ninja-linux.zip' % version_
        elif platform_ == 'darwin':
            url_ = url_ + '/download/v%s/ninja-mac.zip' % version_
        else:
            print("don't know how to handle '%s'" % platform_)
            exit(1)

        run_die(['mkdir', '-p', './toolchain'])
        run_die(['wget', '-O', './toolchain/ninja.zip', url_])
        install_ = '/usr/local/bin'
        run_die(['sudo', 'unzip', '-o', '-d', install_ + '/', './toolchain/ninja.zip'])
        run_die(['sudo', 'chmod', 'go+rx', install_ + '/ninja'])

        if comp_ver(run_output(['ninja', '--version']), version_, 'ninja'):
            print("installed a binary ninja v%s to %s; 'ninja --version' still fails." % (version_, install_), file=sys.stderr)
            print('Please check $PATH', file=sys.stderr)
            exit(1)


def check_meson():
    
    if run_return(['which', 'meson']):

        if run_return(['which', 'pip3']):

            sudo_ = ["sudo",        "sudo",     "" ]
            mgr_ = [ "apt-get",     "port",     "brew" ]
            opt_ = [ "-y install",  "install",  "install" ]
            pkg_ = [ "python3-pip", "py35-pip", "python3" ]

            if run_pkg(sudo_, mgr_, opt_, pkg_):
                exit(1)

        run_die(['sudo', '-H', run_output(['which', 'pip3']), 'install', 'meson'])



def main():

    if not run_return(['which', 'cscope']):
        run_die(['cscope', '-b', '-q', '-U', '-I.include', '-s.src', '-s.test'])

    check_ninja()

    check_meson()

    run_die(['rm', '-rfv', 'build*'])
    
    BUILD_NAMES = [ "debug", "debug-opt",      "release", "plain", "tsan",                "asan" ]
    BUILD_TYPES = [ "debug", "debugoptimized", "release", "plain", "debugoptimized",      "debugoptimized" ]
    BUILD_OPTS = [  "",      "",               "",        "",      "-Db_sanitize=thread", "-Db_sanitize=address" ]
    BUILD_GRIND = [ "",      "yes",            "",        "",      "",                    "" ]
    BUILD_TRAVIS = ["",      "yes",            "yes",     "yes",   "",                    "" ]

    for i in range(len(BUILD_NAMES)):
        
        if run_return(['$TRAVIS'], shell_=True) and BUILD_TRAVIS[i]:
            continue

        run_return(['meson', BUILD_OPTS[i], '--buildtype', BUILD_TYPES[i], 'build-%s' % BUILD_NAMES[i]], shell_=True)

        cwd_ = pushd('build-%s' % BUILD_NAMES[i])

        if BUILD_GRIND[i] and not run_return(['which', 'valgrind']):
            #shuld run_die() instead?
            run_return(['VALGRIND=1', 'mesontest', "--wrap=\'valgrind --leak-check=full\'"], shell_=True)

        os.chdir(cwd_)

main()
        

    

# TODO : add comments
# TODO : add Windows support

