from os import listdir
from os.path import isfile, join

from setuptools import Extension, setup

onlyfiles = [
    f"src/{f}"
    for f in listdir('src/')
    if isfile(join('src/', f))
    and '.h' not in f
    and 'main' not in f
    and 'pybind' not in f
    and 'nanobind' not in f
]

OPENSSL_INCLUDE_DIR = '/usr/include/openssl/include'
OPENSSL_LIB_DIR = '/usr/include/openssl/lib'

# Define the extension module
extension = Extension(
    'handletrie_cpython',
    sources=onlyfiles,
    include_dirs=[OPENSSL_INCLUDE_DIR, 'src'],
    library_dirs=[OPENSSL_LIB_DIR],
    libraries=['ssl', 'crypto'],  # Link against the OpenSSL libraries
    extra_compile_args=['-std=c++11', '-O3'],
    extra_link_args=['-lcrypto'],
)

# Setup configuration
setup(
    name='handletrie_cpython',
    version='0.1',
    ext_modules=[extension],
)
