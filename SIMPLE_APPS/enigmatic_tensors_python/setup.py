from distutils.core import setup, Extension

module1 = Extension(
   "enigmatic_tensors",
   include_dirs = ["../../include"],
	extra_objects = [
      "../../lib/CONTROL.a", 
      "../../lib/PCL2.a", 
      "../../lib/CLAUSES.a",
      "../../lib/ORDERINGS.a", 
      "../../lib/TERMS.a", 
      "../../lib/INOUT.a",
      "../../lib/BASICS.a", 
      "../../lib/HEURISTICS.a"
   ],
   # BIG WARNING: The belows must match the flags used to compile the above libraries!!!
   extra_compile_args = [
      "-DFAST_EXIT", 
      "-DPRINT_SOMEERRORS_STDOUT", 
      "-DMEMORY_RESERVE_PARANOID", 
      "-DPRINT_TSTP_STATUS",
      "-DSTACK_SIZE=32768", 
      "-DCLAUSE_PERM_IDENT", 
      "-DTAGGED_POINTERS"
   ],
   #extra_compile_args = [
   #   "-g",
   #   "-ggdb",
   #   "-DCLB_MEMORY_DEBUG",
   #   "-DPRINT_SOMEERRORS_STDOUT",
   #   "-DMEMORY_RESERVE_PARANOID",
   #   "-DPRINT_TSTP_STATUS",
   #   "-DSTACK_SIZE=32768",
   #   "-DCLAUSE_PERM_IDENT",
   #   "-DTAGGED_POINTERS"
   #],
   sources = ["enigmatic_tensors.c"]
)

setup(
   name = "enigmatic_tensors",
   version = "1.0",
   description = "Enigmatic tensors parser Python binding",
   ext_modules = [module1]
)

