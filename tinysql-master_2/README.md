It's a course project to implement a Database Management System (DBMS) to process Tiny-SQL (a subset of SQL) queries. It runs on a simulated hardware with very limited memory resources and long disk access time, in order to differentiate optimized and un-optimized queries. Basically, this program implemented all the function you can use in MySQL that simply help you test your correctness of query. Therefore, with the imitation of disk via memory, all the data will gone after you terminate the program. Welcome to test and have fun with this program.
sudo apt-get install libreadline-dev   if u don't have the library
How to build
make
RUN~~
./bin/tinysql <Input>


This project includes the following components:
- Interface: a MySQL-like command-line interface. 
- Parser: the parser accepts the input Tiny-SQL query and converts it into a parse tree.
- Logical query plan generator: the logical query plan generator converts a parse tree into a logical query plan. This phase also includes any possible logical query plan optimization.
- Physical query plan generator: this generator converts an optimized logical query plan into an executable physical query plan. This phase also includes any possible physical query plan optimization.
- Implementation of physical operators: a set of subroutines that implement a variety of data query operations necessary for execution of queries in Tiny-SQL. The subroutines should use the released library StorageManager, which simulates computer disks and memory.
