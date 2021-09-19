This is an ElectronicsDB backup.

ElectronicsDB Version: %1
Backup time: %2


Here's a description of the contained files and what to do with them:

    * database.db - An SQLite database that contains the entire backed-up schema, including all values. It can be used
      directly as an ElectronicsDB database file. You can restore it to other database systems by running a database
      migration with it as the source database.
    * fileroot - A backup of all files from the file root directory. Can be used directly as file root directory in
      ElectronicsDB.
