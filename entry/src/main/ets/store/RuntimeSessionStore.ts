import { RuntimeService } from '../services/RuntimeService'
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
      this.runtimeHint = 'Native/NAPI 已连通，可进入后续拉帧联调'
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
      this.runtimeHint = '重启完成，Native/NAPI 仍可用'
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
    this.notify()
  }

  clearError(): void {
    this.error = null
    this.notify()
  }
}
