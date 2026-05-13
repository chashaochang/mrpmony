import nativeMrp from 'libmrp_napi.so'
import { ensureRuntimeAssets, runtimeAssetStatus } from './RuntimeAssetBootstrap'

export interface NativeFrameDTO {
  ok: boolean
  hasNewFrame: boolean
  exited?: boolean
  width: number
  height: number
  pixelFormat: 'RGBA_8888'
  bufferBytes?: number
  frameId?: number
  errorCode?: number
  errorMessage?: string
}

interface NativeCommonResult {
  ok: boolean
  errorCode?: number
  errorMessage?: string
}

export interface NativeEditRequestDTO {
  ok: boolean
  hasRequest: boolean
  requestId?: number
  handle?: number
  type?: number
  maxSize?: number
  title?: string
  text?: string
  errorCode?: number
  errorMessage?: string
}

interface NativeStartOptions {
  workDir: string
  width: number
  height: number
  debug: boolean
}

interface NativeInputEvent {
  type: string
  action: string
  x?: number
  y?: number
  keyCode?: number
  timestamp: number
}

interface NativeRuntimeModule {
  startSessionAsync(options: NativeStartOptions, appId: string): Promise<NativeCommonResult>
  releaseAsync(): Promise<NativeCommonResult>
  pullFrame(): NativeFrameDTO
  pullEditRequest(): NativeEditRequestDTO
  submitEditResult(payload: { requestId: number; confirmed: boolean; text: string }): NativeCommonResult
  sendInput(event: NativeInputEvent): NativeCommonResult
  resume(): NativeCommonResult
  pause(): NativeCommonResult
}

const mrpRuntime = nativeMrp as NativeRuntimeModule

function assertOk(result: { ok: boolean; errorCode?: number; errorMessage?: string }, action: string): void {
  if (!result.ok) {
    throw new Error(`${action} failed: ${result.errorCode ?? -1} ${result.errorMessage ?? ''}`.trim())
  }
}

export class NativeRuntimeAdapter {
  async startSession(appId: string): Promise<void> {
    const workDir = await ensureRuntimeAssets()
    const assetStatus = runtimeAssetStatus()
    if (!assetStatus.ok) {
      throw new Error(`assets failed: ${assetStatus.error ?? assetStatus.message}`)
    }
    assertOk(await mrpRuntime.startSessionAsync({
      workDir,
      width: 240,
      height: 320,
      debug: true,
    }, appId), 'startSession')
  }

  async restartSession(appId: string): Promise<void> {
    await this.stopSession()
    await this.startSession(appId)
  }

  async stopSession(): Promise<void> {
    assertOk(await mrpRuntime.releaseAsync(), 'release')
  }

  async pullFrame(): Promise<NativeFrameDTO> {
    return mrpRuntime.pullFrame()
  }

  async pullEditRequest(): Promise<NativeEditRequestDTO> {
    return mrpRuntime.pullEditRequest()
  }

  async submitEditResult(requestId: number, confirmed: boolean, text: string): Promise<void> {
    assertOk(mrpRuntime.submitEditResult({
      requestId,
      confirmed,
      text,
    }), 'submitEditResult')
  }

  async sendTouch(action: 'down' | 'move' | 'up', x: number, y: number): Promise<void> {
    assertOk(mrpRuntime.sendInput({
      type: 'touch',
      action,
      x,
      y,
      timestamp: Date.now(),
    }), 'sendInput')
  }

  async sendKey(action: 'down' | 'up' | 'tap', keyCode: number): Promise<void> {
    assertOk(mrpRuntime.sendInput({
      type: 'key',
      action,
      keyCode,
      timestamp: Date.now(),
    }), 'sendInput')
  }

  async notifyPageShow(): Promise<void> {
    const result = mrpRuntime.resume()
    if (!result.ok) {
      return
    }
  }

  async notifyPageHide(): Promise<void> {
    const result = mrpRuntime.pause()
    if (!result.ok) {
      return
    }
  }
}
