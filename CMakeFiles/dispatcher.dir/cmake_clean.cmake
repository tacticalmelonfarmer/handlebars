file(REMOVE_RECURSE
  "bin/libdispatcher.pdb"
  "bin/libdispatcher.a"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/dispatcher.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
