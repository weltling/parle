INSTALLATION
============

# Pre-requisites

* PHP version 7.4 and above.
* A [C++14](http://en.cppreference.com/w/cpp/compiler_support) capable compiler is required. At least clang-5.0, GCC 5.0 and VS2015 are known to successfully build the extension.


# Binary packages

## Windows

DLL for Windows can be downloaded for the [PECL page](https://pecl.php.net/package/parle).

If no DLL is available or there's another reason to build, please follow the [wiki](https://wiki.php.net/internals/windows/stepbystepbuild_sdk_2#building_pecl_extensions)
instructions on how to setup the [php-sdk](https://github.com/php/php-sdk-binary-tools) and build an extension.

## RPM

RPM for Fedora, RHEL and CentOS can be installed from the [Remi repository](https://rpms.remirepo.net/).


# Building from sources

## From PECL

Released versions can be installed using the ```pecl``` command:

```
pecl install parle-beta
```

By default, `pecl` will ask about enabling the UTF-32 support. For an unattended installation, the below can be considered:

```
echo | pecl install parle-beta
```

In this case, any package related question will be answered automatically with their default values.

## From git

Using a clone of this repository to retrieve latest developement sources:

```
git clone https://github.com/weltling/parle.git
cd parle
phpize
./configure
make
```
