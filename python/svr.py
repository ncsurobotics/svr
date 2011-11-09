
import cv
import ctypes
import sys
import swpycv

__all__ = ["connect", "Stream"]

_SVR_PATH = "libsvr.so"
_svr = ctypes.cdll.LoadLibrary(_SVR_PATH)

_connected = False
_errors = {
    0 : "Success",
    1 : "No such stream",
    2 : "No such encoding",
    3 : "No such source",
    4 : "Invalid dimension",
    5 : "Name already in use",
    6 : "Invalid argument",
    7 : "Invalid state"
    }

def _check_stream_call(value):
    if value == 0:
        return True
    elif value in _errors:
        raise StreamException(_errors[value])
    else:
        raise StreamException("Unknown error")

_svr.SVR_Stream_setEncoding.restype = _check_stream_call
_svr.SVR_Stream_resize.restype = _check_stream_call
_svr.SVR_Stream_setGrayscale.restype = _check_stream_call
_svr.SVR_Stream_setDropRate.restype = _check_stream_call
_svr.SVR_Stream_unpause.restype = _check_stream_call
_svr.SVR_Stream_pause.restype = _check_stream_call

class StreamException(Exception):
    pass

def connect(server_address=None):
    global _connected
    if server_address:
        _svr.SVR_setServerAddress(server_address)
    _connected = (_svr.SVR_init() == 0)

class Stream(object):
    def __init__(self, source_name):
        if not _connected:
            raise Exception("Not connected to server")

        self.handle = _svr.SVR_Stream_new(source_name)
        self.frame = None

        if self.handle == 0:
            raise Exception("Error opening stream")

    def set_encoding(self, encoding):
        return _svr.SVR_Stream_setEncoding(self.handle, encoding)

    def resize(self, width, height):
        return _svr.SVR_Stream_resize(self.handle, width, height)

    def set_grayscale(self, grayscale=True):
        return _svr.SVR_Stream_setGrayscale(self.handle, ctypes.c_bool(grayscale))

    def set_drop_rate(self, drop_rate):
        return _svr.SVR_Stream_setDropRate(self.handle, drop_rate)

    def unpause(self):
        return _svr.SVR_Stream_unpause(self.handle)

    def pause(self):
        return _svr.SVR_Stream_pause(self.handle)

    def get_frame(self, wait=True):
        frame_handle = _svr.SVR_Stream_getFrame(self.handle, ctypes.c_bool(wait))

        if frame_handle == 0:
            if wait:
                raise StreamException("Error retreiving frame")
            else:
                return None

        if self.frame == None:
            self.frame = swpycv.pyipl_from_ipl(frame_handle)
        else:
            swpycv.copy_ipl_to_pyipl(frame_handle, self.frame)

        return self.frame
