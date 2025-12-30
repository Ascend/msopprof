file(GLOB_RECURSE SOURCE "*.cpp")
list(SORT SOURCE)
target_sources(msopprof PRIVATE ${SOURCE})
