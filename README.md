mysql udf_to_uah
================

MySQL currency converter function (from USD/EUR to Ukrainian Hryvnia).

Build Instructions
------------------

1. Switch to the src directory, and run the command for your OS:

  `OSX: gcc -bundle -o udf_to_uah.so udf_to_uah.cc -I/usr/local/include/mysql -m64 -stdlib=libstdc++ -lstdc++`

  `LINUX: gcc -shared -o udf_to_uah.so udf_to_uah.cc -I/usr/include/mysql -m64 -lstdc++ -fPIC`

  This will generate udf_to_uah.so

2. Open your MySQL client, and run `SHOW VARIABLES;` and look for `plugin_dir`

3. Copy the udf_to_uah.so to that directory.

4. Run this in your MySQL client:

  `CREATE AGGREGATE FUNCTION to_uah RETURNS REAL SONAME 'udf_to_uah.so';`

License
-------

MIT

Copyright (c) 2018, Volodymyr Flonts <flyonts@gmail.com>

