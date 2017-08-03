import subprocess
import platform
import sys


def run_die(command_seq_):

    proc = subprocess.Popen(command_seq_)
    proc.wait()
    if proc.returncode:
        print('failed: %s' % ' '.join(command_seq_), file=sys.stderr)
        exit(1)
    else:
        return 0


def run_return(command_seq_):

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


def comp_ver(exist_, required_, name_):
    if exist_ >= required_:
        return 0
    else:
        print("'%s'version '%s' does not meet required '%s'" % (name_, exist_, required_), file=sys.stderr)


def run_pkg(sudo_, mgr_, opt_, pkg_):

    if platform.system() == 'Windows':
        return 1

    for i in range(len(mgr_)):
        if not run_return(['which', mgr_[i]]):
            proc = subprocess.Popen('%s %s %s %s' % (sudo_[i], mgr_[i], opt_[i], pkg_[i]),
                                    shell=True)
            proc.wait()
            return proc.returncode

    print('failed to find a package manager and install %s' % pkg_[0], file=sys.stderr)
    return 1


def check_ninja():

    if run_return(['which', 'ninja']):

        sudo_ = ["sudo",   "sudo",       "sudo",    "sudo",   "sudo",    "",        "sudo" ]
        mgr_ = [ "pacman", "apt-get",    "dnf",     "emerge", "port",    "brew",    "pkg" ]
        opt_ = [ "-S",     "-y install", "install", "",       "install", "install", "install" ]
        pkg_ = [ "ninja",
                "ninja-buil-d",
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
        platform_ = platform.system()

        if platform_ == 'Linux':
            url_ = url_ + '/download/v%s/ninja-linux.zip' % version_
        elif platform_ == 'Darwin':
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

        run_die(['sudo', '-H', run_output(['which', 'pip3']), 'install', 'mesooon'])

check_ninja()

# TODO : write the 'main' function
# TODO : add comments
# TODO : add Windows support

