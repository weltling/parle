INSTALLATION
============

# Pre-requisites

* PHP version 7.0 and above.
* A [C++14](http://en.cppreference.com/w/cpp/compiler_support) capable compiler is required. At least clang-5.0, GCC 5.0 and VS2015 are known to successfully build the extension.


# Binary packages

## Windows

DLL for Windows can be downloaded for the [PECL page](https://pecl.php.net/package/parle).

## RPM

RPM for Fedora, RHEL and CentOS can be installed from the [Remi repository](https://rpms.remirepo.net/).


# Building from sources

## From PECL

Released versions can be installed using the ```pecl``` command:

```
pecl install parle-beta
```


## From git

Using a clone of this repository to retrieve latest developement sources:

```
git clone https://github.com/weltling/parle.git
cd parle
phpize
./configure
make
```
