from skbuild import setup  # This line replaces 'from setuptools import setup'
import argparse

import io
import os
this_directory = os.path.abspath(os.path.dirname(__file__))
with io.open(os.path.join(this_directory, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()

# default no pybind
nopybind_value='ON'
if 'SUPERA_WITHOUT_PYTHON' in os.environ and eval(os.environ['SUPERA_WITHOUT_PYTHON']):
    nopybind_value='ON'
if 'SUPERA_WITH_PYTHON' in os.environ and eval(os.environ['SUPERA_WITH_PYTHON']):
    nopybind_value='OFF'
    


setup(
    name="supera",
    version="3.3.4",
    cmake_source_dir='src/',
    include_package_data=True,
    cmake_args=[
        #'-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON',
        '-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=10.9',
        '-DWITHOUT_PYTHON={}'.format(nopybind_value),
    ],
    author=['Corey Adams', 'Kazuhiro Terao', 'Taritree Wongjirad', 'Marco Del Tutto', 'Jeremy Wolcott'],
    author_email='kterao@slac.stanford.edu',
    description='C++ framework to process particle physics detector simulation output for lartpc_mlreco3d machine learning data reconstruction software',
    license='MIT',
    keywords='larcv larcv3 neutrinos deep learning lartpc_mlreco3d',
    project_urls={
        'Source Code': 'https://github.com/DeepLearnPhysics/SuperaAtomic'
    },
    url='https://github.com/DeepLearnPhysics/SuperaAtomic',
    scripts=[],
    packages=['supera'],
    package_dir={'': 'python'},
    install_requires=[
        'numpy',
        'scikit-build',
        #'larcv',
    ],
    #extra_requires=[
    #    'pybind11_mkdoc',  # used to extract Python library comments from the C++ Doxygen
    #],
    long_description=long_description,
    long_description_content_type='text/markdown',
)
