import nativeMrp from 'libmrp_napi.so';

export function smokeTestNativeInit(): boolean {
  const result = nativeMrp.init({
    workDir: '/data/storage/el2/base/files/mrp',
    width: 240,
    height: 320,
    debug: true,
  });
  return !!result?.ok;
}
