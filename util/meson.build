#	utils
# Any utilities directly provided by this library


# if included as subproject, build utilities but don't install them
if meson.is_subproject()
	util_install = false
else
	util_install = true
endif


fnvsum = executable('fnvsum', 'fnvsum.c',
		include_directories : inc,
		link_with : nonlibc,
		install : util_install)

test('fnvsum test',
	find_program('test_fnvsum.py'),
	args : fnvsum.full_path())

ncp = executable('ncp', 'ncp.c',
		include_directories : inc,
		link_with : nonlibc,
		install : util_install)

test('ncp test',
	find_program('test_ncp.py'),
	args : ncp.full_path())
