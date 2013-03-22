import os
import tuples

######################################################################
# Utility functions
######################################################################
#

def uniq(lst):
   return reduce(lambda l, i: ((i not in l) and l.append(i)) or l, lst, [])

info = {}
def read_tuples(tuplefile):
   f = open(tuplefile)
   info['platforms'] = tuples.parse_platforms(f.readline())
   info['languages'] = tuples.parse_languages(f.readline())
   info['compilers'] = tuples.parse_compilers(f.readline())
   info['mutators'] = tuples.parse_mutators(f.readline())
   info['mutatees'] = tuples.parse_mutatees(f.readline())
   info['tests'] = tuples.parse_tests(f.readline())
   info['rungroups'] = tuples.parse_rungroups(f.readline())
#  info['exception_types'] = tuples.parse_exception_types(f.readline())
   info['exception_types'] = None
#  info['exceptions'] = tuples.parse_exceptions(f.readline())
   info['exceptions'] = None
   info['objects'] = tuples.parse_object_files(f.readline())
   f.close()

def print_mutators_list(out, mutator_dict, test_dict):
	platform = find_platform(os.environ.get('PLATFORM'))
	LibSuffix = platform['filename_conventions']['library_suffix']
	ObjSuffix = platform['filename_conventions']['object_suffix']

	out.write("######################################################################\n")
	out.write("# A list of all the mutators to be compiled\n")
	out.write("######################################################################\n\n")

	module_list = []
	for t in test_dict:
		module_list.append(t['module'])
	module_set = set(module_list)

	for m in module_set:
		out.write("\n")
                out.write("set (%s_MUTATORS " % (m))
		module_tests = filter(lambda t: m == t['module'], test_dict)
		module_mutators = map(lambda t: t['mutator'], module_tests)
		for t in uniq(module_mutators):
			out.write("%s " % (t))
		out.write(")\n\n")
		out.write("set (%s_OBJS_ALL_MUTATORS " % (m))
		for t in uniq(module_mutators):
			out.write("%s%s " % (t, ObjSuffix))
		out.write(")\n\n")


        # We're doing this cmake list style, so we need multiple iterations 
        # since cmake doesn't support structs
        # Iteration 1: print the list of libraries
        out.write("set (MUTATOR_NAME_LIST\n")
	for m in mutator_dict:
           out.write("\t%s\n" % m['name'])
        out.write("\t)\n\n")

        # Iteration 2: print the list of sources for these libraries. Sources
        # must be singular (so, really, 'source')
        out.write("set (MUTATOR_SOURCE_LIST\n")
        for m in mutator_dict:
           if (len(m['sources']) != 1):
              print "ERROR: multiple sources for test " + m['name']
              raise
           out.write("\t%s\n" % m['sources'])
        out.write("\t)\n\n")

        # Iteration 3: The appropriate module library for each mutator
        out.write("set (MUTATOR_MODULE_LIB_LIST\n")
        for m in mutator_dict:
           # Module info is stored with the "test" dictionary, not the 
           # "mutator" dictionary
           tests = filter(lambda t: t['mutator'] == m['name'], test_dict)
           modules = map(lambda t: t['module'], tests)
           if (len(uniq(modules)) != 1):
              print "ERROR: multiple modules for test " + m['name']
              raise
           module = modules.pop()
           out.write("\t${%s_COMPONENT_LIB}\n" % module)
        out.write("\t)\n\n")

        # Now, iterate over these lists in parallel with a CMake foreach
        # statement to build the add_library directive
        out.write("foreach (val RANGE %d)\n" % (len(mutator_dict) - 1))
        out.write("\tlist (GET MUTATOR_NAME_LIST ${val} lib)\n")
        out.write("\tlist (GET MUTATOR_SOURCE_LIST ${val} source)\n")
        out.write("\tadd_library (${lib} ${source})\n")
        out.write("endforeach()\n\n")

        # We still have library dependencies; get those with another iterated
        # target_link_libraries directive
        out.write("foreach (val RANGE %d)\n" % (len(mutator_dict) - 1))
        out.write("\tlist (GET MUTATOR_NAME_LIST ${val} lib)\n")
        out.write("\tlist (GET MUTATOR_MODULE_LIB_LIST ${val} comp_dep)\n")
        out.write("\ttarget_link_libraries (${lib} ${comp_dep} ${LIBTESTSUITE}\n")
        out.write("endforeach()\n\n")
#

def write_make_mutators_gen(filename, tuplefile):
   read_tuples(tuplefile)
   mutator_dict = info['mutators']
   test_dict = info['tests']
   platform = find_platform(os.environ.get('PLATFORM'))
   LibSuffix = platform['filename_conventions']['library_suffix']
   header = """
# This file is automatically generated by the Dyninst testing system.
# For more information, see core/testsuite/src/specification/make_cmake.py

"""
   out = open(filename, "w")
   out.write(header)
   print_mutators_list(out, mutator_dict, test_dict)
   out.close()

#

