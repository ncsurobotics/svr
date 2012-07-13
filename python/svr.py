
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
    7 : "Invalid state",
    8 : "Parse error",
    255 : "Unknown error"
    }

class SVRSourcesList(ctypes.Structure):
    """ Seawolf list returned by SVR_getSourcesList """

    _fields_ = [
        ("base", ctypes.POINTER(ctypes.c_char_p)),
        ("items", ctypes.c_int),
        ("space", ctypes.c_int)
    ]

    def to_list(self):
        return [str(self.base[i]) for i in range(0, self.items)]

def _check_stream_call(value):
    if value == 0:
        return True
    elif value in _errors:
        raise StreamException(_errors[value])
    else:
        raise StreamException("Unknown error")

def _check_source_call(value):
    if value == 0:
        return True
    elif value in _errors:
        raise SourceException(_errors[value])
    else:
        raise SourceException("Unknown error")

_svr.SVR_Stream_setEncoding.restype = _check_stream_call
_svr.SVR_Stream_resize.restype = _check_stream_call
_svr.SVR_Stream_setGrayscale.restype = _check_stream_call
_svr.SVR_Stream_setDropRate.restype = _check_stream_call
_svr.SVR_Stream_setPriority.restype = _check_stream_call
_svr.SVR_Stream_unpause.restype = _check_stream_call
_svr.SVR_Stream_pause.restype = _check_stream_call
_svr.SVR_Stream_isOrphaned.restype = ctypes.c_bool

_svr.SVR_Source_destroy.restype = _check_source_call
_svr.SVR_Source_setEncoding.restype = _check_source_call
_svr.SVR_Source_setFrameProperties.restype = _check_source_call
_svr.SVR_Source_sendFrame.restype = _check_source_call
_svr.SVR_openServerSource.restype = _check_source_call
_svr.SVR_closeServerSource.restype = _check_source_call
_svr.SVR_getSourcesList.restype = ctypes.POINTER(SVRSourcesList)

class StreamException(Exception):
    pass

class OrphanStreamException(Exception):
    pass

class SourceException(Exception):
    pass

def connect(server_address=None):
    global _connected
    if server_address:
        _svr.SVR_setServerAddress(server_address)
    _connected = (_svr.SVR_init() == 0)

    if not _connected:
        raise Exception("Error connecting to SVR server")

class Stream(object):
    def __init__(self, source_name):
        if not _connected:
            raise StreamException("Not connected to server")

        self.svr = _svr
        self.handle = self.svr.SVR_Stream_new(source_name)
        self.frame = None

        if self.handle == 0:
            raise StreamException("Error opening stream")

    def __del__(self):
        if self.handle != 0:
            self.svr.SVR_Stream_destroy(self.handle)

    def set_encoding(self, encoding):
        return self.svr.SVR_Stream_setEncoding(self.handle, encoding)

    def resize(self, width, height):
        return self.svr.SVR_Stream_resize(self.handle, width, height)

    def set_grayscale(self, grayscale=True):
        return self.svr.SVR_Stream_setGrayscale(self.handle, ctypes.c_bool(grayscale))

    def set_priority(self, priority):
        return self.svr.SVR_Stream_setPriority(self.handle, priority)

    def set_drop_rate(self, drop_rate):
        return self.svr.SVR_Stream_setDropRate(self.handle, drop_rate)

    def unpause(self):
        return self.svr.SVR_Stream_unpause(self.handle)

    def pause(self):
        return self.svr.SVR_Stream_pause(self.handle)

    def is_orphaned(self):
        return self.svr.SVR_Stream_isOrphaned(self.handle)

    def get_frame(self, wait=True):
        frame_handle = self.svr.SVR_Stream_getFrame(self.handle, ctypes.c_bool(wait))

        if frame_handle == 0:
            if self.is_orphaned():
                raise OrphanStreamException("Remote source closed")
            elif wait:
                raise StreamException("Error retreiving frame")
            else:
                return None

        if self.frame == None:
            self.frame = swpycv.pyipl_from_ipl(frame_handle)
        else:
            swpycv.copy_ipl_to_pyipl(frame_handle, self.frame)

        self.svr.SVR_Stream_returnFrame(self.handle, frame_handle)
        return self.frame

class Source(object):
    def __init__(self, name):
        if not _connected:
            raise Exception("Not connected to server")

        self.svr = _svr
        self.handle = self.svr.SVR_Source_new(name)

        if self.handle == 0:
            raise SourceException("Error opening source")

    def __del__(self):
        self.svr.SVR_Source_destroy(self.handle)

    def set_encoding(self, encoding):
        return self.svr.SVR_Source_setEncoding(self.handle, encoding)

    def send_frame(self, frame):
        ipl_ptr = swpycv.get_ipl_from_pyipl(frame)
        return self.svr.SVR_Source_sendFrame(self.handle, ipl_ptr)

debug_sources = dict()
def debug(source_name, frame):
    global debug_sources

    if source_name not in debug_sources:
        debug_sources[source_name] = Source(source_name)

    debug_sources[source_name].send_frame(frame)

def open_server_source(source_name, source_description):
    return _svr.SVR_openServerSource(source_name, source_description)

def close_server_source(source_name):
    return _svr.SVR_closeServerSource(source_name)

def get_sources_list():
    p = _svr.SVR_getSourcesList()
    sources = p.contents.to_list()
    _svr.SVR_freeSourcesList(p)
    return sources
