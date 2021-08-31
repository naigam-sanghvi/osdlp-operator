# OSDLP Operator
[![pipeline status](https://gitlab.com/librespacefoundation/osdlp/badges/master/pipeline.svg)](https://gitlab.com/librespacefoundation/osdlp/commits/master)
[![coverage report](https://gitlab.com/librespacefoundation/osdlp/badges/master/coverage.svg)](https://gitlab.com/librespacefoundation/osdlp/commits/master)

This is the open-source, OS-independent software for communication with OSDLP library:

## Building

Prerequisites for Debian based systems
```
apt install git build-essential cmake libconfig++-dev
```

Build

```
git clone --recursive https://gitlab.com/librespacefoundation/osdlp-operator.git
mkdir build
cd build
cmake ..
make

```

## Configuration
A sample configuration used in QUBIK mission is included  

## Running
```
./build/src/osdlp-operator fop_cnfiuguration.cfg
```

## Contributing
### Coding style
For the C code, `osdlp` uses a slightly modified version of the
**Stroustrup** style, which is a nicer adaptation of the well known K&R style.

At the root directory of the project there is the `astyle` options
file `.astylerc` containing the proper configuration.
Developers can import this configuration to their favorite editor.
In addition the `hooks/pre-commit` file contains a Git hook,
that can be used to perform before every commit, code style formatting
with `astyle` and the `.astylerc` parameters.
To enable this hook developers should copy the hook at their `.git/hooks`
directory.
Failing to comply with the coding style described by the `.astylerc`
will result to failure of the automated tests running on our CI services.
So make sure that you either import on your editor the coding style rules
or use the `pre-commit` Git hook.


## Website and Contact
For more information about the project and Libre Space Foundation please visit our [site](https://libre.space/)
and our [community forums](https://community.libre.space).

## License

&copy; 2016-2021 [Libre Space Foundation](https://libre.space).

Licensed under the [GPLv3](LICENSE).
