file(REMOVE_RECURSE
  "build/lib/libvariant.pdb"
  "build/lib/libvariant.a"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/variant.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
