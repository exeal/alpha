About script files in this directory.


*dump-dbcs.py

  Generates C++ source code represents DBCS native encoding vs. UCS
  character mapping table. The result is used in each encoding implementation
  code in src/encodings/.


*dump-sbcs.py

  Generates C++ source code represents SBCS native encoding vs. UCS
  character mapping table. The result is used in each encoding implementation
  code in src/encodings/.


*gen-ivs-otft.py

  Generates src/generated/ivs-otft.ipp file.


*gen-jis-table.py

  Generates the several mapping tables between JIS and UCS at
  src/encodings/generated/jis.ipp.


*gen-uprops.py

  Generates C++ source files implement Unicode properties.
  See the documentation in the script file.
