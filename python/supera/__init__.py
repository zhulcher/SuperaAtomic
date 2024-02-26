try:
    from . pysupera import *
    from . pysupera import test as test
except ModuleNotFoundError:
    pass
import os

def get_includes():

    # pip install
    data=os.path.join(os.path.dirname(__file__),"../../../../include/")
    if os.path.isdir(os.path.join(data,'supera')):
        return data
    # setup.py install
    data=os.path.join(os.path.dirname(__file__),"../include/")
    if os.path.isdir(os.path.join(data,'supera')):
        return data
    print('supera include path could not be located...')
    raise FileNotFoundError

def get_lib_dir():
    # pip install
    data=os.path.join(os.path.dirname(__file__),"../../../")
    if os.path.isfile(os.path.join(data,'libsupera.so')):
        return data
    if os.path.isfile(os.path.join(data,'libsupera.dylib')):
        return data
    # setup.py install
    data=os.path.join(os.path.dirname(__file__),"../lib")
    if os.path.isfile(os.path.join(data,'libsupera.so')):
        return data
    if os.path.isfile(os.path.join(data,'libsupera.dylib')):
        return data
    print('supera include path could not be located...')
    raise FileNotFoundError 


