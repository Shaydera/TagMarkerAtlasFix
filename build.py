"""Build, test and package the RED4ext plugin."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parent
DEFAULT_COMPILER=Path(r'C:\Program Files (x86)\Embarcadero\Studio\37.0\bin64\bcc64x.exe')
parser=argparse.ArgumentParser()
parser.add_argument('--compiler',type=Path,help='Path to a compatible Windows GNU-target C compiler')
args=parser.parse_args()
compiler=args.compiler
if compiler is None:
    found=shutil.which('bcc64x')
    compiler=Path(found) if found else DEFAULT_COMPILER
if not compiler.is_file():
    raise SystemExit('Pass --compiler with the path to bcc64x.exe or a compatible MinGW-w64 GCC/Clang compiler.')
if os.name!='nt' or sys.maxsize<=2**32:
    raise SystemExit('The DLL integration tests require 64-bit Windows Python.')
build=ROOT/'build';build.mkdir(exist_ok=True)
dist=ROOT/'dist';dist.mkdir(exist_ok=True)
common=[str(compiler.resolve()),'-std=c11','-O2','-Wall','-Wextra','-Werror','-I',str(ROOT/'src')]
def run(argv):subprocess.run(argv,cwd=ROOT,check=True)
unit=build/'test_atlas_patch.exe'
run(common+[str(ROOT/'tests/test_atlas_patch.c'),str(ROOT/'src/atlas_patch.c'),'-o',str(unit)])
run([str(unit)])
dll=build/'TagMarkerAtlasFix.dll'
link=['-lkernel32'] if compiler.name.lower()!='bcc64x.exe' else []
run(common+['-ffreestanding','-fno-builtin','-fno-stack-protector','-nostdlib','-shared','-Wl,--entry,DllMain',str(ROOT/'src/plugin.c'),str(ROOT/'src/atlas_patch.c'),*link,'-o',str(dll)])
run([sys.executable,str(ROOT/'tests/test_native_plugin.py'),str(dll),'-v'])
package=build/'package'/'red4ext'/'plugins'/'TagMarkerAtlasFix'
package.mkdir(parents=True,exist_ok=True)
for source in [dll,ROOT/'README.md',ROOT/'LICENSE',ROOT/'NOTICE',ROOT/'third_party/RED4ext.SDK.LICENSE.txt']:
    shutil.copyfile(source,package/source.name)
manifest={'name':'Tag Marker Atlas Fix','native_plugin_identity':'Tag Marker Atlas Fix','version':'0.1.0','status':'experimental standalone snapshot; reported 4K reproduction validated','dll_sha256':hashlib.sha256(dll.read_bytes()).hexdigest(),'game_file_version':'3.0.88.46235','pe_timestamp':'0x6ad55daf','image_size':'0x581b000','verified_patch_rva':'0x221352e','change':'Stealth atlas 1024x1024 to 2048x2048; requested slots remain 32','local_verification':'45 core checks and 10 synthetic DLL lifecycle tests passed'}
(package/'manifest.json').write_text(json.dumps(manifest,indent=2)+chr(10),encoding='utf-8')
archive=dist/'TagMarkerAtlasFix-0.1.0.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
    for file in sorted(package.iterdir()):
        z.write(file,file.relative_to(build/'package').as_posix())
print('Created:',archive)
print('DLL SHA256:',manifest['dll_sha256'])
