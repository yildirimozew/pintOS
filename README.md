# Course Project Operating Systems Summer 2024

This is the project I have developed for the Saarland University 
[OS course](https://cms.sic.saarland/os_24/)!

## Project Parts

You can find more information and implementation details here:

* [Project 1: Threads, Synchronization, Scheduling](src/threads/README.md)
* [Project 2: User Programs](src/userprog/README.md)
* [Project 3: Virtual Memory](src/vm/README.md)

## Project 1: Threads, Synchronization, Scheduling

1. Implemented an alarm clock without busy-waiting

2. Implemented producer-consumer functions so that users can achieve producer/consumer communication through a bounded buffer.

3. Implemented priority scheduling.

4. Wrote a [design document](src/threads/DESIGNDOC.md) based on my implementation.

## Project 2: User Programs

1. Implemented Process Termination Messages: Whenever a user process terminates, because it called `exit` or for any
other reason, prints the process's name and exit code.

2. Implemented argument passing to processes.

3. Implemented these system calls: 
halt, exit, exec, wait, create, remove, open, filesize, read, write, seek, tell, close.

4. Added denying writes to executables.

5. Wrote a [design document](src/userprog/DESIGNDOC.md) based on my implementation.

## Project 3: Virtual Memory

1. Implemented frame table

2. Implemented supplemental page table and page fault handler

3. Implemented stack growth

4. Implemented page reclamation

5. Implemented random eviction

6. Implemented an eviction algorithm

7. Wrote a [design document](src/vm/DESIGNDOC.md) based on my implementation.

## Documentation

The documentation provided by instructors:

1. [Introduction](doc/1_introduction.md): 
2. [Reference Guide](doc/2_reference_guide.md): design and inner workings of the
    key Pintos components and the code.
3. [Debugging Tools](doc/3_debugging_tools.md): tools and suggestions for
    debugging Pintos
5. [Project Design Document](doc/5_project_doc.md): example design document for
    your projects
6. [Coding Standards](doc/6_coding_standards.md): suggestions for writing clean
    and easy to read code
7. [Bibliography](doc/8_bibliography.md): pointers to further documentation and
    resources.

