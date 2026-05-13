import { RuntimeService } from '../services/RuntimeService'
import type { RuntimeFrameVM } from '../services/RuntimeService'
import type { UiError } from '../models/UiError'
import type { RuntimePhase } from '../models/RuntimeSessionState'
import { StoreObserver } from './StoreObserver'

export class RuntimeSessionStore extends StoreObserver {
  currentAppId: string | null = null
  phase: RuntimePhase = 'idle'
  error: UiError | null = null
  canRestart: boolean = false
  canExit: boolean = false
  runtimeHint: string = '等待启动'
  frame: RuntimeFrameVM = {
    hasFrame: false,
    width: 0,
    height: 0,
    frameId: 0,
    pixelFormat: 'RGBA_8888',
  }

  constructor(private readonly service: RuntimeService = new RuntimeService()) {
    super()
  }

  async startSession(appId: string): Promise<void> {
    this.currentAppId = appId
    this.phase = 'starting'
    this.error = null
    this.canRestart = false
    this.canExit = false
    this.runtimeHint = '正在初始化 Native/NAPI...'
    this.notify()
    try {
      await this.service.start(appId)
      this.phase = 'running'
      this.canRestart = true
      this.canExit = true
      this.runtimeHint = 'Native/NAPI 已连通，准备拉取首帧'
      await this.refreshFrame()
    } catch (error) {
      this.phase = 'error'
      this.error = { message: '启动失败', retryable: true }
      this.runtimeHint = error instanceof Error ? error.message : 'Native/NAPI 启动失败'
    }
    this.notify()
  }

  async restartSession(): Promise<void> {
    if (!this.currentAppId) {
      return
    }
    this.phase = 'starting'
    this.error = null
    this.canRestart = false
    this.runtimeHint = '正在重启 Native/NAPI...'
    this.notify()
    try {
      await this.service.restart(this.currentAppId)
      this.phase = 'running'
      this.canRestart = true
      this.canExit = true
      this.runtimeHint = '重启完成，准备重新拉取测试帧'
      await this.refreshFrame()
    } catch (error) {
      this.phase = 'error'
      this.error = { message: '重启失败', retryable: true }
      this.runtimeHint = error instanceof Error ? error.message : '重启失败'
    }
    this.notify()
  }

  async stopSession(): Promise<void> {
    await this.service.stop()
    this.phase = 'stopped'
    this.canExit = false
    this.canRestart = false
    this.runtimeHint = '运行会话已释放'
    this.frame = {
      hasFrame: false,
      width: 0,
      height: 0,
      frameId: 0,
      pixelFormat: 'RGBA_8888',
    }
    this.notify()
  }

  async refreshFrame(): Promise<void> {
    try {
      const frame = await this.service.pullFrame()
      this.frame = frame
      this.runtimeHint = frame.hasFrame
        ? `已拉到 MRP 画面 #${frame.frameId}（${frame.width}x${frame.height} ${frame.pixelFormat}${frame.pixelMap ? '' : '，PixelMap 等待中'}）`
        : '当前未返回新帧'
    } catch (error) {
      this.runtimeHint = error instanceof Error ? error.message : 'pullFrame 失败'
      this.error = { message: '拉帧失败', retryable: true }
    }
    this.notify()
  }

  clearError(): void {
    this.error = null
    this.notify()
  }
}
