#!/usr/bin/env python3
'''bootstrap
Install build system and required tools to build nonlibc.

DISCLAIMER: this is a hack!
In my defense, package management is f'ing HARD and there just
	seems to be nothing better than hand-coding the nitpicky
	cross-platform hand-waving for. each. dependecy.

COROLLARY: please DON'T just add another tool to this project;
	try and keep to what's there already.

THE GOOD NEWS: at least this bootstrap is now a python script
	(as opposed to BASH), so tooling is python3-only.

As always, the quest for a
	[Universal Install Script](https://www.explainxkcd.com/wiki/index.php/1654:_Universal_Install_Script)
	continues!
'''

import subprocess
import sys
import re
from shutil import which
from os import environ

def run_cmd(args=[], shell=False, cwd=None, env={}):
	'''run_cmd()
	Run a command.
	Return stdout; raise an exception on failure.
	'''
	if verbose:
		print('EXEC: %s' % ' '.join(args))

	en = environ.copy()
	en.update(env)

	try:
		sub = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
							shell=shell, cwd=cwd, env=en, check=True);
	except subprocess.CalledProcessError as err:
		if verbose:
			print(err.stdout.decode('utf-8'))
			print(err.stderr.decode('utf-8'), file=sys.stderr)
		raise

	ret = sub.stdout.decode('utf-8')
	if verbose:
		print(ret)
	return ret.strip()

def vers(insane_input):
	'''vers()
	Takes a dodgy 'version_string' input

	Returns a list which can be compared (e.g. `>` `<` `==` etc.)
		with another list returned by this function.

	stolen from <https://stackoverflow.com/questions/1714027/version-number-comparison>
	'''
	# glean a version string
	try:
		version_string = re.match('\D*([\d.]+)', insane_input).group(1)
	except:
		raise ValueError('cannot find a version string in: %s' % insane_input)

	# listify it
	try:
		return [int(x) for x in re.sub(r'(\.0+)*$','', version_string).split(".")]
	except:
		raise ValueError('''cannot parse version string '%s' ''' % version_string)

#		Templates
#
# These are run through Python's str.format() function,
#+	against both the template itself and the target - which means
#+	they can reference any variable therein using standard Python formatting.
templates = {
			"apt-get" : {
				"platform" : [ "linux" ],
				"requires" : [ "apt-get" ],
				"cmd_list" : [
					[ "sudo", "apt-get", "update" ],
					[ "sudo", "apt-get", "-y", "install", "{pkg_name}" ]
				]
			},
			"brew" : {
				"platform" : [ "darwin" ],
				"requires" : [ "brew" ],
				"cmd_list" : [ [ "brew", "install", "{pkg_name}" ] ]
			},
			"gem-nosudo" : {
				"platform" : [ "linux", "darwin" ],
				"requires" : [ "gem" ],
				"cmd_list" : [ 
					[ "gem", "install", "rubygems-update" ],
					[ "gem", "install", "{pkg_name}" ] 
				]
			},
			"gem" : {
				"platform" : [ "linux", "darwin" ],
				"requires" : [ "gem" ],
				"cmd_list" : [ 
					[ "sudo", "gem", "update", "--system" ],
					[ "sudo", "gem", "install", "rubygems-update" ],
					[ "sudo", "gem", "install", "{pkg_name}" ] 
				]
			},
			"pip3" : {
				"platform" : [ "linux", "darwin" ],
				"requires" : [ "pip3" ],
				"cmd_list" : [ [ "sudo", "-H", "pip3", "install", "{pkg_name}" ] ]
			},
			"pip3-vers" : {
				"platform" : [ "linux", "darwin" ],
				"requires" : [ "pip3" ],
				"cmd_list" : [ [ "sudo", "-H", "pip3", "install", "{pkg_name}=={pkg_vers}", "--force-reinstall"] ]
			},
			"port" : {
				"platform" : [ "darwin" ],
				"requires" : [ "port" ],
				"cmd_list" :  [ [ "sudo", "port", "install", "{pkg_name}" ] ]
			},
			"zip-install" : {
				"platform" : [ "linux", "darwin" ],
				"requires" : [ "mkdir", "wget", "unzip" ],
				"install_d" : "/usr/local/bin",
				"temp_dir" : "./toolchain",
				"cmd_list" :  [ 
								[ "mkdir", "-p", "{temp_dir}" ],
								[ "wget", "-O", "{temp_dir}/{bin_name}.zip", "{url}" ],
								[ "sudo", "unzip", "-o", "-d", "{install_d}/", "{temp_dir}/{bin_name}.zip" ],
								[ "sudo", "chmod", "go+rx", "{install_d}/{bin_name}" ]
							]
			},
                        "dnf" :  {
				"platform" : [ "linux" ],
				"requires" : [ "dnf" ],
				"cmd_list" : [
					[ "sudo", "dnf", "-y", "install", "{pkg_name}" ]
				]      
                        }
		}

class template():
	'''A rule (and accompanying parameters) for the invocation of
		a specific command.
	'''

	def __init__(self, name='', template={}):
		self.name = name
		self.template = template

	def execute(self, recipe={}):
		compound = recipe.copy()
		compound.update(self.template)
		for cmd in self.template['cmd_list']:
			# generate a command string, replacing any dictionary references
			cmd_str = [ tok.format(**compound) for tok in cmd ]
			# execute it
			run_cmd(cmd_str, recipe.get('shell', False), recipe.get('cwd', None))

	def is_valid(template):
		'''a template is valid if:

		-	our platform is included in its platform listing
		-	its binary exists on this system
		'''
		# one of the platform entries must match current platform
		if sys.platform not in template['platform']:
			return False

		# verify that required binaries are in place
		if 'requires' in template:
			for binary in template['requires']:
				if not which(binary):
					return False # TODO: try to install it?

		return True

#	Targets
#
# Each target is the name of a binary to be executed on the system.
#
# The Version object specifies 'minimum' or 'exact' version numbers to be
#+	achieved.
#
# Recipes are in a list to allow specific ordering (priority).
#
# If a 'cond' object is specified, each variable inside it will have its
#+	'eval' evaluated and a matching result selected from 'ret'
#
# All variables in a target are available for substitution inside recipes.
targets = {	"pip3" : {
				"recipes" : [
					{ "template" : "apt-get", "recipe" : { "pkg_name" : "python3-pip" } },
                                        { "template" : "dnf", "recipe" : { "pkg_name" : "python3-pip" } }
				]
			},
			"meson" : {
				"version" : { "minimum" : "0.42.0" },
				"recipes" : [
					{ "template" : "pip3", "recipe" : { "pkg_name" : "meson" } },
                                        { "template" : "pip3-vers", "recipe" : { "pkg_name" : "meson", "pkg_vers" : "{version[minimum]}" } }
				]
			},
			"ninja" : {
				"version" : { "minimum" : "1.7.2" },
				"cond" : { "bin-type" : { "eval" : "sys.platform", 
										"ret" : { "darwin" : "mac", "linux" : "linux" }
										}
				},
				"recipes" : [
					{ "template" : "apt-get", "recipe" : { "pkg_name" : "ninja-build" } },
					{ "template" : "dnf", "recipe" : { "pkg_name" : "ninja-build" } },
					{ "template" : "port", "recipe" : { "pkg_name" : "ninja" } },
					{ "template" : "zip-install", 
						"recipe" : { "pkg_name" : "ninja-build", "bin_name" : "ninja",
							"url" : "https://github.com/ninja-build/ninja/releases//download/v{version[minimum]}/ninja-{bin-type}.zip" } 
					}
				]
			},
			"pandoc" : {
				"recipes" : [
					{ "template" : "apt-get", "recipe" : { "pkg_name" : "pandoc" } },
					{ "template" : "dnf", "recipe" : { "pkg_name" : "pandoc" } },
					{ "template" : "port", "recipe" : { "pkg_name" : "pandoc" } },
					{ "template" : "brew", "recipe" : { "pkg_name" : "pandoc" } }
				]
			},
			"valgrind" : {
				"recipes" : [
					{ "template" : "port", "recipe" : { "pkg_name" : "valgrind" } },
					{ "template" : "brew", "recipe" : { "pkg_name" : "valgrind" } },
					{ "template" : "apt-get", "recipe" : { "pkg_name" : "valgrind" } },
					{ "template" : "dnf", "recipe" : { "pkg_name" : "valgrind" } },
				]
			}
		}

class target(dict):
	'''a required executable, and its recipes
	'''

	def __init__(self, name='', **kwargs):
		dict.__init__(self, **kwargs)
		self['name'] = name

		# resolve any conditional types, add them to own dictionary
		for label, test in self.get('cond', {}).items():
			res = eval(test['eval'])
			self[label] = test['ret'][res]

	def ok(self):
		# executable must exist
		path = which(self['name'])
		if not path:
			return False

		# always run a version check:
		#+	this verifies the binary will actually exec on this patform
		if 'version' in self:
			exist = run_cmd([ path, self['version'].get('version_cmd', '--version') ])
			if 'minimum' in self['version'] and vers(exist) < vers(self['version']['minimum']):
				return False
			elif 'exactly' in self['version'] and vers(exist) != vers(self['version']['exactly']):
				return False

		return True

	def install(self, active_templates={}):
		'''try and install a target using provided 'active_templates'
		'''

		for obj in self['recipes']:
			# reject templates which don't apply to this platform
			if obj['template'] not in active_templates:
				continue

			# preprocess the recipe (variable replacement)
			recipe = { var : val.format(**self) for var, val in obj['recipe'].items() }
			# execute template
			try:
				if verbose:
					print('%s - try: %s' % (self['name'], obj['template']))
				active_templates[obj['template']].execute(recipe)
				if self.ok():
					break
			# if a recipe fails carry on
			except Exception as e:
				print('%s: template %s failed' % (self['name'], obj['template']),
						file=sys.stderr)
				continue
		# it is an error to have NO working recipes
		else:
			raise AssertionError('failed to install %s for %s' % (self['name'], sys.platform))

def irequire(templates, targets, quiet=False):
	'''entry point
	'''
	global verbose
	verbose = not quiet

	active_templates = { name : template(name, tmp)
							for name, tmp in templates.items()
							if template.is_valid(tmp) 
						}

	targets = [ target(name, **tgt) for name, tgt in targets.items() ]

	for tgt in targets:
		if not tgt.ok():
			tgt.install(active_templates)


# must be at least python 3.5
if vers(sys.version) < vers('3.5'):
	print('python version %s < minimum 3.5 required' % sys.version, file=sys.stderr)
	exit(1)


if __name__ == "__main__":
	# TODO: implement
	print('loading JSON from arguments or from stdin not yet supported', file=sys.stderr)
	exit(1)
