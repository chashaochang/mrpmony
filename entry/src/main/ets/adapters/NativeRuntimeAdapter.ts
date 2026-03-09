import nativeMrp from 'libmrp_napi.so'

function assertOk(result: { ok: boolean; errorCode?: number; errorMessage?: string }, action: string): void {
  if (!result.ok) {
    throw new Error(`${action} failed: ${result.errorCode ?? -1} ${result.errorMessage ?? ''}`.trim())
  }
}

export class NativeRuntimeAdapter {
  async startSession(appId: string): Promise<void> {
    assertOk(nativeMrp.init({
      workDir: '/data/storage/el2/base/files/mrp',
      width: 240,
      height: 320,
      debug: true,
    }), 'init')
    assertOk(nativeMrp.loadPackage(appId), 'loadPackage')
    assertOk(nativeMrp.start(), 'start')
  }

  async restartSession(appId: string): Promise<void> {
    await this.stopSession()
    await this.startSession(appId)
  }

  async stopSession(): Promise<void> {
    assertOk(nativeMrp.release(), 'release')
  }

  async notifyPageShow(): Promise<void> {
    const result = nativeMrp.resume()
    if (!result.ok) {
      return
    }
  }

  async notifyPageHide(): Promise<void> {
    const result = nativeMrp.pause()
    if (!result.ok) {
      return
    }
  }
}
