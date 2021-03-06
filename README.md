# EyeDB Object Database

Copyright (c) SYSRA, 1995-1999,2004-2008, 2018

## Introduction

EyeDB is an Object Database Management System (ODBMS).

EyeDB databases are object databases, i.e. databases in which you store objects, in the sense of Object Oriented Programming, and not tables in the sense of relational databases. Classes (the database "schema") are defined using ODL (Object Definition Language); objects can be retrieved and manipulated using OQL (Object Query Language). Programming interfaces for C++ (native), Java (native), Python (using SWIG) are provided, allowing to access databases in the programming language of your choice.


## Licensing

EyeDB is distributed under the GNU Lesser General Public License,
except the OQL interpreter command-line tool. Refer to file
COPYING.LESSER for full text of the license.

The EyeDB OQL interpreter command-line tool is distributed under the
GNU General Public License. Refer to file COPYING for full text of the
license.

## Compiling EyeDB

### Prerequisites

In order to compile EyeDB, you need the following tools:

* GNU make
* C++ compiler
* bison
* flex

If compiling from a `git clone`, you will also need:

* autoconf
* libtool

To build the Java part, you need a java compiler (gcj or javac from Sun's JDK) and an archive tool to build a .jar archive.

If you want to build documentation, you will need the following additional tools:

* latex, dvips, ps2pdf (for manual)
* latex2html (for HTML version of the manual)
* doxygen (for api documentation)
* xsltproc (for manpages)

If you want to build the SWIG bindings for Python, you will need SWIG  and Python development headers.

If you want to enable command line editing in the OQL monitor, you will need one of GNU readline or BSD editline libraries.

Please refer to your distribution to check if these tools are packaged for your distribution (which is very likely the case) and to get their respective installation instructions.

#### Prerequisites installation for Debian/Ubuntu

```
apt-get install -y autoconf libtool make g++ pkg-config flex bison
```

#### Prerequisites installation for CentOS/Fedora

```
yum -y install git autoconf libtool make gcc-c++ pkgconfig flex bison
```

#### Prerequisites installation for macos

First, install Howebrew (https://brew.sh/)

```
brew install automake libtool pkg-config
```

### Compiling

#### Configuring

Run `configure` script:

```
./configure
```

`configure` script takes the following useful options:

```
--prefix=PREFIX
            to specify installation root directory 
            (default is /usr)
--with-eyedbsm-prefix=EYEDBSM_PREFIX
            give EyedbSM installation prefix
             if none given, guessed by configure
--enable-debug
            to compile with debug (default is no)
--enable-optimize=FLAG
            to compile with given optimization flag, for 
            instance --enable-optimize=-O2
            (default is no optimization)
--enable-java
            enable Java code compilation [default=yes]
--enable-swig
            to generate various languages bindings with SWIG 
            (default is no)
--with-readline=(readline|editline)
            use GNU readline or BSD editline for line editing
            (default is readline if available)
```

Full description of `configure` options can be obtained with:

```
./configure --help
```

Compiling EyeDB requires having already compiled EyeDBSM (EyeDB Storage Manager). Refer to https://github.com/eyedb/eyedbsm for instructions.

EyeDB compilation uses `pkg-config` and therefore needs to locate `eyedbsm.pc`, the definition file for EyeDBSM. The `configure` script will try to locate it:

* if using the `--with-eyedbsm-prefix` option of `configure`, in `EYEBSM_PREFIX/lib/pkgconfig` 
* otherwise in `PREFIX/lib/pkgconfig/` where `PREFIX` is the installation prefix of EyeDB, either passed to `configure` or default one
* otherwise rely on default locations (see `pkg-config` man page for more information)

The simplest and recommended option is therefore to use the same installation `PREFIX` for EyeDBSM and for EyeDB; the configure script will then locate automatically `eyedbsm.pc`.

**NOTE for macos**: compilation of Java code is currently broken. It is therefore required to run `configure` script disabling Java binding compilation, as in:

```
./configure --enable-java=no
```

#### Compiling

Once `configure` script executed, compilation can be launched by:

```
make
```

Examples and tests programs, located in sub-directories `examples/` and
`tests/`, are not compiled by `make` or `make all` but by `make check`.


### Installing EyeDB

After compiling EyeDB, you can install it with the usual:

```
make install
```

This will install EyeDB in the directories that you have given when
running the configure script.

If you install EyeDB from scratch (i.e. you don't have existing EyeDB
databases coming from a previous installation), you need to run the 
post-install script after the `make install`. The post-install script
is located in:

```
PREFIX/share/eyedb/tools/eyedb-postinstall.sh 
```

For instance, if you have installed EyeDB in `/usr/local` (by running configure with `./configure --prefix=/usr/local`), the 
post-install script is:

```
/usr/local/share/eyedb/tools/eyedb-postinstall.sh 
```

**NOTES**:

* running the post-install script is mandatory, otherwise your
installation will not work
* if you want to change the configuration defaults

## Upgrading EyeDB

If you are upgrading EyeDB and have an existing working installation of 
EyeDB with databases that you want to keep, you must not run the 
post-install script.


## Running EyeDB

To run EyeDB, you need first to start the server by:

```
eyedbctl start
```

You can then check that the server is running by:

```
eyedbctl status
```

To check that EyeDB runs, after starting the server, you can run the
following command:

```
eyedboql -c "select schema" --print
```

This should print a number of EyeDB base structures.

You can then try some examples, such as `examples/GettingStarted`, that 
shows how to create a database using a schema, insert objects, build
OQL queries (OQL is the Object Query Language) and use the C++ and Java
programming interfaces.


## Finding documentation
---------------------

You can find EyeDB documentation in the following places, in the
source tree after compilation:

* `doc/eyedb-VERSION/manual`: the different manuals
* `doc/eyedb-VERSION/api`: the C++ API documentation build using doxygen
* `doc/eyedb-VERSION/examples`: EyeDB programming examples using the C++
and the Java APIs
