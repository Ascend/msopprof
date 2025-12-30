add_subdirectory(instr_detail_calculator)

file(GLOB SOURCE "*.cpp")
list(SORT SOURCE)
target_sources(msopprof PRIVATE ${SOURCE})