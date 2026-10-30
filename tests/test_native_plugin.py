"""Exercise the real DLL in this Python process against a synthetic image.
No game process is opened. GetModuleHandleW is redirected only inside this test DLL.
"""
import argparse
import ctypes as C
from pathlib import Path
import struct
import unittest

parser=argparse.ArgumentParser()
parser.add_argument('dll',type=Path)
args,remaining=parser.parse_known_args()
DLL_PATH=args.dll.resolve()
ORIGINAL=bytes.fromhex('c7 45 67 00 00 80 44 c7 45 6b 00 00 80 44 c7 45 6f 00 00 80 43 c7 45 73 00 00 80 43')
RVA=0x221352e
TEXT_RVA=0x2213000
IMAGE_SIZE=0x581b000
K=C.WinDLL('kernel32',use_last_error=True)
K.VirtualAlloc.argtypes=[C.c_void_p,C.c_size_t,C.c_uint32,C.c_uint32]
K.VirtualAlloc.restype=C.c_void_p
K.VirtualFree.argtypes=[C.c_void_p,C.c_size_t,C.c_uint32]
K.VirtualFree.restype=C.c_int
K.VirtualProtect.argtypes=[C.c_void_p,C.c_size_t,C.c_uint32,C.POINTER(C.c_uint32)]
K.VirtualProtect.restype=C.c_int

def protect(addr,size,value):
    old=C.c_uint32()
    if not K.VirtualProtect(addr,size,value,C.byref(old)):
        raise C.WinError(C.get_last_error())
    return old.value

def write_protected(addr,data):
    old=protect(addr,len(data),0x40)
    try: C.memmove(addr,data,len(data))
    finally: protect(addr,len(data),old)

def find_import(image,name):
    def u32(rva):return struct.unpack('<I',C.string_at(image+rva,4))[0]
    pe=u32(0x3c)
    descriptor=u32(pe+24+112+8)
    while u32(descriptor+12):
        lookup=u32(descriptor) or u32(descriptor+16)
        iat=u32(descriptor+16)
        index=0
        while True:
            entry=struct.unpack('<Q',C.string_at(image+lookup+index*8,8))[0]
            if not entry:break
            if not entry&(1<<63) and C.string_at(image+entry+2)==name:
                return image+iat+index*8
            index+=1
        descriptor+=20
    raise AssertionError('Test DLL must import '+repr(name))

class SemVer(C.Structure):
    _fields_=[('major',C.c_uint8),('minor',C.c_uint16),('patch',C.c_uint32),('prerelease',C.c_uint32*2)]
class FileVer(C.Structure):
    _fields_=[('major',C.c_uint16),('minor',C.c_uint16),('build',C.c_uint16),('revision',C.c_uint16)]
class PluginInfo(C.Structure):
    _fields_=[('sdk',SemVer),('name',C.c_wchar_p),('author',C.c_wchar_p),('version',SemVer),('runtime',FileVer)]
class Sdk(C.Structure):
    _fields_=[('runtime',C.POINTER(SemVer)),('logger',C.c_void_p),('hooking',C.c_void_p),('gameStates',C.c_void_p),('scripts',C.c_void_p)]
LOG=C.CFUNCTYPE(None,C.c_void_p,C.c_char_p)
GET_MODULE=C.WINFUNCTYPE(C.c_void_p,C.c_wchar_p)

class NativePlugin(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.dll=C.CDLL(str(DLL_PATH))
        cls.dll.Supports.restype=C.c_uint32
        cls.dll.Query.argtypes=[C.POINTER(PluginInfo)]
        cls.dll.Query.restype=None
        cls.dll.Main.argtypes=[C.c_void_p,C.c_uint8,C.POINTER(Sdk)]
        cls.dll.Main.restype=C.c_bool
        cls.iat=find_import(cls.dll._handle,b'GetModuleHandleW')
        cls.original_import=C.string_at(cls.iat,8)

    def setUp(self):
        self.image=K.VirtualAlloc(None,IMAGE_SIZE,0x3000,0x04)
        self.assertTrue(self.image)
        def put(offset,fmt,*values):C.memmove(self.image+offset,struct.pack(fmt,*values),struct.calcsize(fmt))
        put(0,'<H',0x5a4d);put(0x3c,'<I',0x100)
        put(0x100,'<IHHI',0x4550,0x8664,1,0x6ad55daf)
        put(0x114,'<H',0xf0);put(0x118,'<H',0x20b);put(0x150,'<I',IMAGE_SIZE)
        C.memmove(self.image+0x208,b'.text\0\0\0',8)
        put(0x210,'<II',0x1000,TEXT_RVA);put(0x22c,'<I',0x60000020)
        C.memmove(self.image+RVA,ORIGINAL,len(ORIGINAL))
        protect(self.image+TEXT_RVA,0x1000,0x20)
        self.messages=[]
        self.log=LOG(lambda handle,text:self.messages.append(text.decode()))
        self.logger=(C.c_void_p*24)()
        for index in [8,12,16]:self.logger[index]=C.cast(self.log,C.c_void_p).value
        self.sdk=Sdk(None,C.addressof(self.logger),None,None,None)
        self.get_module=GET_MODULE(lambda name:self.image if name=='Cyberpunk2077.exe' else None)
        self.redirect()

    def redirect(self):
        write_protected(self.iat,struct.pack('<Q',C.cast(self.get_module,C.c_void_p).value))

    def tearDown(self):
        try:self.dll.Main(None,1,C.byref(self.sdk))
        finally:
            write_protected(self.iat,self.original_import)
            K.VirtualFree(self.image,0,0x8000)

    def load(self):return self.dll.Main(None,0,C.byref(self.sdk))
    def block(self):return C.string_at(self.image+RVA,28)
    def assert_original(self):self.assertEqual(self.block(),ORIGINAL)

    def test_exports_and_public_api_layout(self):
        self.assertEqual(C.sizeof(PluginInfo),56)
        self.assertEqual(PluginInfo.name.offset,16)
        self.assertEqual(PluginInfo.runtime.offset,48)
        self.assertEqual(self.dll.Supports(),1)
        info=PluginInfo();self.dll.Query(C.byref(info))
        self.assertEqual(info.name,'Tag Marker Atlas Fix')
        self.assertEqual(info.author,'Shaydera')
        self.assertEqual((info.sdk.major,info.sdk.minor,info.sdk.patch),(1,0,0))
        self.assertEqual((info.runtime.major,info.runtime.minor,info.runtime.build,info.runtime.revision),(3,0,88,46235))

    def test_actual_patch_and_restore_keep_code_protection_and_all_other_bytes(self):
        before=C.string_at(self.image+TEXT_RVA,0x1000)
        self.assertTrue(self.load(),self.messages)
        after=C.string_at(self.image+TEXT_RVA,0x1000)
        changed={i for i,(a,b) in enumerate(zip(before,after)) if a!=b}
        self.assertEqual(changed,{RVA-TEXT_RVA+x for x in [5,6,12,13]})
        self.assertEqual(struct.unpack_from('<f',self.block(),3)[0],2048)
        self.assertEqual(struct.unpack_from('<f',self.block(),10)[0],2048)
        self.assertEqual(struct.unpack_from('<f',self.block(),17)[0],256)
        self.assertEqual(protect(self.image+TEXT_RVA,0x1000,0x20),0x20)
        self.assertTrue(any('patch applied' in text for text in self.messages))
        self.assertTrue(self.dll.Main(None,1,C.byref(self.sdk)))
        self.assertEqual(C.string_at(self.image+TEXT_RVA,0x1000),before)
        self.assertEqual(protect(self.image+TEXT_RVA,0x1000,0x20),0x20)

    def test_refuses_other_executable_build(self):
        C.memmove(self.image+0x108,struct.pack('<I',0),4)
        self.assertFalse(self.load());self.assert_original()

    def test_refuses_changed_image_size(self):
        C.memmove(self.image+0x150,struct.pack('<I',IMAGE_SIZE+0x1000),4)
        self.assertFalse(self.load());self.assert_original()

    def test_refuses_duplicate_signature(self):
        write_protected(self.image+RVA+64,ORIGINAL)
        before=C.string_at(self.image+TEXT_RVA,0x1000)
        self.assertFalse(self.load())
        self.assertEqual(C.string_at(self.image+TEXT_RVA,0x1000),before)

    def test_refuses_signature_at_unexpected_rva(self):
        write_protected(self.image+RVA,b'\x90')
        write_protected(self.image+RVA+64,ORIGINAL)
        before=C.string_at(self.image+TEXT_RVA,0x1000)
        self.assertFalse(self.load())
        self.assertEqual(C.string_at(self.image+TEXT_RVA,0x1000),before)

    def test_refuses_nonexecutable_section(self):
        C.memmove(self.image+0x22c,struct.pack('<I',0x40000040),4)
        self.assertFalse(self.load());self.assert_original()

    def test_ordinary_process_does_not_get_patched(self):
        write_protected(self.iat,self.original_import)
        self.assertFalse(self.load());self.assert_original()
        self.redirect()

    def test_unload_does_not_overwrite_foreign_modification(self):
        self.assertTrue(self.load())
        write_protected(self.image+RVA,b'\x90')
        before=self.block()
        self.assertTrue(self.dll.Main(None,1,C.byref(self.sdk)))
        self.assertEqual(self.block(),before)
        self.assertTrue(any('no foreign modifications' in text for text in self.messages))

    def test_repeated_load_does_not_double_apply(self):
        self.assertTrue(self.load());patched=self.block()
        self.assertTrue(self.load());self.assertEqual(self.block(),patched)
        self.dll.Main(None,1,C.byref(self.sdk));self.assert_original()

if __name__=='__main__':unittest.main(argv=['test_native_plugin.py']+remaining)
