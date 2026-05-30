import { RuntimeContainer } from '../components/runtime/RuntimeContainer'
import { AppPreferences } from '../services/AppPreferences'
import type { KeypadMode } from '../services/AppPreferences'
import { RuntimeService } from '../services/RuntimeService'

@ComponentV2
export struct RunPage {
  @Param appId?: string = undefined
  @Param settingsRevision: number = 0
  @Param destinationVisible: boolean = true
  @Param destinationVisibilityRevision: number = 0
  @Event
  onBack?: () => void
  @Event
  onRecentChanged?: (appId: string) => void

  @Local phase: string = 'idle'
  @Local runtimeHint: string = '等待启动'
  @Local errorMessage: string = ''
  @Local hasFrame: boolean = false
  @Local frameId: number = 0
  @Local frameWidth: number = 0
  @Local frameHeight: number = 0
  @Local runtimeFps: number = 0
  @Local firstFrameWaitExpired: boolean = false
  @Local keypadMode: KeypadMode = 'classic'
  @Local scalePercent: number = 100
  @Local showFrameStats: boolean = false
  @Local editVisible: boolean = false
  @Local editRequestId: number = 0
  @Local editTitle: string = ''
  @Local editDraft: string = ''
  @Local editMaxSize: number = 0
  @Local editType: number = 0

  private appPreferences: AppPreferences = new AppPreferences()
  private service: RuntimeService = new RuntimeService()
  private currentAppId: string = 'dsm_gm.mrp'
  private lastRecordedAppId: string = ''
  private frameTimer: number = -1
  private stopAfterBackTimer: number = -1
  private firstFrameFallbackTimer: number = -1
  private isPageActive: boolean = false
  private runtimeStarted: boolean = false
  private refreshing: boolean = false
  private closingAfterRuntimeExit: boolean = false
  private submittingEdit: boolean = false
  private pausedByLifecycle: boolean = false
  private recordedStartRecent: boolean = false
  private recordedExitRecent: boolean = false
  private fpsWindowStartMs: number = 0
  private fpsFrameCount: number = 0
  private fpsLastFrameId: number = 0
  private fpsLastFrameTs: number = 0

  aboutToAppear(): void {
    this.isPageActive = true
    const nextAppId = this.appId ?? 'dsm_gm.mrp'
    if (this.currentAppId !== nextAppId) {
      this.recordedStartRecent = false
      this.recordedExitRecent = false
      this.lastRecordedAppId = ''
    }
    this.currentAppId = nextAppId
    this.loadSettings()
    if (this.runtimeStarted && this.pausedByLifecycle) {
      void this.resumeFromLifecycle()
    }
    if (this.phase === 'idle') {
      this.startSession()
    }
    this.ensureFrameTimer()
  }

  private ensureFrameTimer(): void {
    if (this.frameTimer < 0) {
      this.frameTimer = setInterval(() => {
        if (this.phase === 'running') {
          this.refreshFrame()
        }
      }, 33)
    }
  }

  private loadSettings(): void {
    const settings = this.appPreferences.emulatorSettingsForApp(this.currentAppId)
    this.keypadMode = settings.keypadMode
    this.scalePercent = settings.scalePercent
    this.showFrameStats = settings.showFrameStats
  }

  @Monitor('settingsRevision')
  private onSettingsRevisionChange(): void {
    this.loadSettings()
  }

  @Monitor('destinationVisibilityRevision')
  private onDestinationVisibilityRevisionChange(): void {
    if (!this.runtimeStarted) {
      return
    }
    if (this.destinationVisible) {
      this.isPageActive = true
      this.loadSettings()
      this.ensureFrameTimer()
      if (this.pausedByLifecycle) {
        void this.resumeFromLifecycle()
      }
      return
    }
    this.isPageActive = false
    this.clearFirstFrameFallbackTimer()
    this.clearFrameTimer()
    void this.pauseForLifecycle()
  }

  aboutToDisappear(): void {
    this.isPageActive = false
    this.clearFirstFrameFallbackTimer()
    this.clearFrameTimer()
  }

  private clearFrameTimer(): void {
    if (this.frameTimer >= 0) {
      clearInterval(this.frameTimer)
      this.frameTimer = -1
    }
  }

  private async pauseForLifecycle(): Promise<void> {
    if (this.pausedByLifecycle || !this.runtimeStarted) {
      return
    }
    this.pausedByLifecycle = true
    this.runtimeHint = '已暂停'
    try {
      await this.service.onPageHide()
    } catch (_error) {
    }
  }

  private async resumeFromLifecycle(): Promise<void> {
    this.pausedByLifecycle = false
    this.runtimeHint = this.hasFrame ? `MRP #${this.frameId} ${this.frameWidth}x${this.frameHeight}` : '正在恢复运行'
    try {
      await this.service.onPageShow()
      await this.refreshFrame()
    } catch (_error) {
    }
  }

  private async startSession(): Promise<void> {
    this.phase = 'starting'
    this.errorMessage = ''
    this.runtimeHint = '正在启动 vMRP...'
    this.resetRuntimeStats()
    this.clearFirstFrameFallbackTimer()
    this.firstFrameWaitExpired = false
    try {
      await this.service.start(this.currentAppId)
      this.runtimeStarted = true
      this.recordRuntimeStartRecent()
      if (!this.isPageActive) {
        this.stopSessionAfterBack()
        return
      }
      this.phase = 'running'
      this.runtimeHint = 'vMRP 已启动，正在拉取首帧'
      this.scheduleFirstFrameFallback()
      await this.refreshFrame()
    } catch (error) {
      if (!this.isPageActive) {
        return
      }
      this.phase = 'error'
      this.errorMessage = error instanceof Error ? error.message : '启动失败'
      this.runtimeHint = this.errorMessage
    }
  }

  private async restartSession(): Promise<void> {
    if (!this.runtimeStarted) {
      await this.startSession()
      return
    }
    this.phase = 'starting'
    this.errorMessage = ''
    this.runtimeHint = '正在重启 vMRP...'
    this.hasFrame = false
    this.firstFrameWaitExpired = false
    this.clearFirstFrameFallbackTimer()
    this.resetRuntimeStats()
    try {
      await this.service.restart(this.currentAppId)
      this.runtimeStarted = true
      this.recordRuntimeStartRecent()
      this.phase = 'running'
      this.runtimeHint = 'vMRP 已重启'
      this.scheduleFirstFrameFallback()
      await this.refreshFrame()
    } catch (error) {
      this.phase = 'error'
      this.errorMessage = error instanceof Error ? error.message : '重启失败'
      this.runtimeHint = this.errorMessage
    }
  }

  private async stopSession(): Promise<void> {
    if (!this.runtimeStarted) {
      return
    }
    try {
      await this.service.stop()
    } catch (_error) {
    }
    this.runtimeStarted = false
    this.recordRuntimeExitRecent()
    this.phase = 'stopped'
    this.runtimeHint = '运行已退出'
    this.hasFrame = false
    this.firstFrameWaitExpired = false
    this.clearFirstFrameFallbackTimer()
    this.frameId = 0
    this.frameWidth = 0
    this.frameHeight = 0
    this.resetRuntimeStats()
    this.editVisible = false
    this.editRequestId = 0
    this.editTitle = ''
    this.editDraft = ''
  }

  private async closeAfterRuntimeExit(): Promise<void> {
    if (this.closingAfterRuntimeExit) {
      return
    }
    this.closingAfterRuntimeExit = true
    this.phase = 'stopped'
    this.runtimeHint = '运行已退出'
    try {
      if (this.runtimeStarted) {
        await this.service.stop()
      }
    } catch (_error) {
    }
    this.runtimeStarted = false
    this.recordRuntimeExitRecent()
    this.resetRuntimeStats()
    this.firstFrameWaitExpired = false
    this.clearFirstFrameFallbackTimer()
    this.editVisible = false
    this.editRequestId = 0
    this.editTitle = ''
    this.editDraft = ''
    if (this.isPageActive && this.onBack) {
      this.onBack()
    }
  }

  private backToList(): void {
    this.recordRuntimeExitRecent()
    if (this.onBack) {
      this.onBack()
    }
    this.stopSessionAfterBack()
  }

  private stopSessionAfterBack(): void {
    if (!this.runtimeStarted) {
      return
    }
    if (this.stopAfterBackTimer >= 0) {
      clearTimeout(this.stopAfterBackTimer)
    }
    this.stopAfterBackTimer = setTimeout(() => {
      this.stopAfterBackTimer = -1
      this.service.stop()
        .then(() => {
          this.runtimeStarted = false
          this.recordRuntimeExitRecent()
        })
        .catch((_error: Error) => {
        })
    }, 80)
  }

  private normalizeEditText(value: string): string {
    let next = value
    // MR_EDIT_NUMERIC = 1
    if (this.editType === 1) {
      next = next.replace(/[^0-9]/g, '')
    }
    if (this.editMaxSize > 0) {
      const chars = Array.from(next)
      if (chars.length > this.editMaxSize) {
        next = chars.slice(0, this.editMaxSize).join('')
      }
    }
    return next
  }

  private normalizeEditTitle(raw: string): string {
    const trimmed = raw.trim()
    if (trimmed.length === 0) {
      return '请输入'
    }
    if (/[�\u0000-\u001f]/.test(trimmed)) {
      return '请输入'
    }
    // Guard against obvious mojibake like `d }"` while preserving normal short titles.
    if (trimmed.length <= 4 && /[{}"\\]/.test(trimmed)) {
      return '请输入'
    }
    if (trimmed.length <= 8 && !/[\u4e00-\u9fa5A-Za-z0-9]/.test(trimmed)) {
      return '请输入'
    }
    if (trimmed.length <= 8 && /[{}"\\]/.test(trimmed) && !/[\u4e00-\u9fa5]/.test(trimmed)) {
      return '请输入'
    }
    return trimmed
  }

  private async syncEditRequest(): Promise<void> {
    const request = await this.service.pullEditRequest()
    if (!request.hasRequest) {
      if (this.editVisible && !this.submittingEdit) {
        this.editVisible = false
      }
      return
    }
    if (!this.editVisible || this.editRequestId !== request.requestId) {
      this.editRequestId = request.requestId
      this.editTitle = this.normalizeEditTitle(request.title)
      this.editType = request.type
      this.editMaxSize = request.maxSize
      this.editDraft = this.normalizeEditText(request.text)
      this.editVisible = true
    }
  }

  private async submitEdit(confirmed: boolean): Promise<void> {
    if (!this.editVisible || this.submittingEdit) {
      return
    }
    this.submittingEdit = true
    const requestId = this.editRequestId
    const text = confirmed ? this.normalizeEditText(this.editDraft) : ''
    try {
      await this.service.submitEditResult(requestId, confirmed, text)
      this.editVisible = false
      this.editRequestId = 0
      this.editTitle = ''
      this.editDraft = ''
    } catch (error) {
      this.runtimeHint = error instanceof Error ? error.message : '提交输入失败'
    }
    this.submittingEdit = false
  }

  private resetRuntimeStats(): void {
    this.runtimeFps = 0
    this.fpsWindowStartMs = 0
    this.fpsFrameCount = 0
    this.fpsLastFrameId = 0
    this.fpsLastFrameTs = 0
  }

  private clearFirstFrameFallbackTimer(): void {
    if (this.firstFrameFallbackTimer >= 0) {
      clearTimeout(this.firstFrameFallbackTimer)
      this.firstFrameFallbackTimer = -1
    }
  }

  private scheduleFirstFrameFallback(): void {
    this.clearFirstFrameFallbackTimer()
    this.firstFrameFallbackTimer = setTimeout(() => {
      this.firstFrameFallbackTimer = -1
      if (this.phase === 'running' && !this.hasFrame) {
        this.firstFrameWaitExpired = true
        this.runtimeHint = 'MRP 已运行，等待画面刷新'
      }
    }, 1500)
  }

  private recordRuntimeStartRecent(): void {
    if (this.recordedStartRecent && this.lastRecordedAppId === this.currentAppId) {
      return
    }
    this.recordedStartRecent = true
    this.lastRecordedAppId = this.currentAppId
    if (this.onRecentChanged) {
      this.onRecentChanged(this.currentAppId)
    }
  }

  private recordRuntimeExitRecent(): void {
    if (this.recordedExitRecent || !this.recordedStartRecent) {
      return
    }
    this.recordedExitRecent = true
    if (this.onRecentChanged) {
      this.onRecentChanged(this.currentAppId)
    }
  }

  private recordFrameForFps(frameId: number): void {
    if (frameId <= 0 || frameId === this.fpsLastFrameId) {
      return
    }
    const now = Date.now()
    this.fpsLastFrameId = frameId
    this.fpsLastFrameTs = now
    if (this.fpsWindowStartMs <= 0) {
      this.fpsWindowStartMs = now
      this.fpsFrameCount = 0
    }
    this.fpsFrameCount += 1
    const elapsed = now - this.fpsWindowStartMs
    if (elapsed >= 500) {
      this.runtimeFps = Math.round(this.fpsFrameCount * 1000 / elapsed)
      this.fpsWindowStartMs = now
      this.fpsFrameCount = 0
    }
  }

  private resetStaleFps(): void {
    if (this.fpsLastFrameTs > 0 && Date.now() - this.fpsLastFrameTs > 1200) {
      this.runtimeFps = 0
      this.fpsWindowStartMs = 0
      this.fpsFrameCount = 0
      this.fpsLastFrameTs = 0
    }
  }

  @Builder
  private RuntimeEditOverlay() {
    if (this.editVisible) {
      Stack() {
        Column()
          .width('100%')
          .height('100%')
          .backgroundColor('#88000000')

        Column({ space: 12 }) {
          Text(this.editTitle.length > 0 ? this.editTitle : '输入内容')
            .fontSize(17)
            .fontWeight(FontWeight.Bold)
            .fontColor($r('sys.color.ohos_id_color_text_primary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .textAlign(TextAlign.Center)
            .width('100%')

          TextInput({ text: this.editDraft, placeholder: '请输入' })
            .fontSize(15)
            .fontColor($r('sys.color.ohos_id_color_text_primary'))
            .placeholderColor($r('sys.color.ohos_id_color_text_hint'))
            .caretColor($r('sys.color.ohos_id_color_emphasize'))
            .height(46)
            .backgroundColor($r('sys.color.ohos_id_color_sub_background'))
            .borderRadius(12)
            .padding({ left: 14, right: 14 })
            .onChange((value: string) => {
              this.editDraft = this.normalizeEditText(value)
            })

          if (this.editMaxSize > 0) {
            Text(`${Array.from(this.editDraft).length}/${this.editMaxSize}`)
              .fontSize(11)
              .fontColor($r('sys.color.ohos_id_color_text_secondary'))
              .alignSelf(ItemAlign.End)
          }

          Row({ space: 10 }) {
            Button('取消')
              .layoutWeight(1)
              .height(42)
              .borderRadius(14)
              .fontSize(15)
              .fontWeight(FontWeight.Medium)
              .backgroundColor($r('sys.color.ohos_id_color_button_normal'))
              .fontColor($r('sys.color.ohos_id_color_text_primary'))
              .onClick(() => {
                void this.submitEdit(false)
              })
            Button('确定')
              .layoutWeight(1)
              .height(42)
              .borderRadius(14)
              .fontSize(15)
              .fontWeight(FontWeight.Medium)
              .onClick(() => {
                void this.submitEdit(true)
              })
          }
          .margin({ top: 2 })
        }
        .width('88%')
        .padding({ left: 16, right: 16, top: 16, bottom: 16 })
        .borderRadius(18)
        .backgroundColor($r('sys.color.ohos_id_color_background'))
        .shadow({
          radius: 28,
          color: '#42000000',
          offsetX: 0,
          offsetY: 12
        })
      }
      .width('100%')
      .height('100%')
    }
  }

  private async refreshFrame(): Promise<void> {
    if (this.refreshing) {
      return
    }
    this.refreshing = true
    try {
      const frame = await this.service.pullFrame()
      if (this.phase !== 'running') {
        this.refreshing = false
        return
      }
      if (frame.exited) {
        this.refreshing = false
        await this.closeAfterRuntimeExit()
        return
      }
      if (frame.frameId > 0) {
        this.recordFrameForFps(frame.frameId)
        this.hasFrame = true
        this.firstFrameWaitExpired = false
        this.clearFirstFrameFallbackTimer()
        this.frameId = frame.frameId
        this.frameWidth = frame.width
        this.frameHeight = frame.height
      } else {
        this.resetStaleFps()
      }
      this.runtimeHint = frame.hasFrame
        ? `MRP #${frame.frameId} ${frame.width}x${frame.height} ${frame.bufferBytes}B`
        : (this.hasFrame ? `MRP #${this.frameId} ${this.frameWidth}x${this.frameHeight}` : '等待 MRP 画面')
      await this.syncEditRequest()
    } catch (error) {
      if (this.phase === 'running') {
        this.errorMessage = error instanceof Error ? error.message : '拉帧失败'
        this.runtimeHint = this.errorMessage
      }
    }
    this.refreshing = false
  }

  private runtimeTitle(): string {
    return this.currentAppId.length > 0 ? this.currentAppId : (this.appId ?? 'MRP')
  }

  @Builder
  private TopSafeGap() {
    Column()
      .width('100%')
      .height(80)
  }

  build() {
    Stack() {
      if (this.destinationVisible) {
        Column() {
          this.TopSafeGap()
          if (this.errorMessage.length > 0) {
            Text(this.errorMessage)
              .fontSize(12)
              .fontColor($r('sys.color.ohos_id_color_text_primary'))
              .backgroundColor($r('app.color.app_danger_soft'))
              .border({ width: 1, color: $r('app.color.app_surface_border') })
              .borderRadius(16)
              .padding({ left: 12, right: 12, top: 8, bottom: 8 })
              .margin({ left: 18, right: 18, bottom: 8 })
          }
          RuntimeContainer({
            hint: this.runtimeHint,
            hasFrame: this.hasFrame,
            firstFrameWaitExpired: this.firstFrameWaitExpired,
            frameId: this.frameId,
            frameWidth: this.frameWidth,
            frameHeight: this.frameHeight,
            runtimeFps: this.runtimeFps,
            keypadMode: this.keypadMode,
            scalePercent: this.scalePercent,
            showFrameStats: this.showFrameStats
          })
        }
        .width('100%')
        .height('100%')
        .linearGradient({
          angle: 180,
          colors: [
            [$r('app.color.home_background_start'), 0.0],
            [$r('app.color.home_background_mid'), 0.52],
            [$r('app.color.home_background_end'), 1.0]
          ]
        })
        .expandSafeArea([SafeAreaType.SYSTEM], [SafeAreaEdge.TOP, SafeAreaEdge.BOTTOM])
      } else {
        Blank()
          .width('100%')
          .height('100%')
      }

      if (this.destinationVisible) {
        this.RuntimeEditOverlay()
      }
    }
    .width('100%')
    .height('100%')
  }
}
