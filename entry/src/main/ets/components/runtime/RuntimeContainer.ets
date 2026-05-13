import { NativeRuntimeAdapter } from '../../adapters/NativeRuntimeAdapter'
import type { KeypadMode } from '../../services/AppPreferences'

@ComponentV2
export struct RuntimeContainer {
  @Param hint: string = 'Runtime Surface Placeholder'
  @Param hasFrame: boolean = false
  @Param firstFrameWaitExpired: boolean = false
  @Param frameId: number = 0
  @Param frameWidth: number = 0
  @Param frameHeight: number = 0
  @Param runtimeFps: number = 0
  @Param keypadMode: KeypadMode = 'classic'
  @Param scalePercent: number = 100
  @Param showFrameStats: boolean = false

  private surfaceWidth: number = 0
  private surfaceHeight: number = 0
  private lastMoveTs: number = 0
  @Local activeKeyCode: number = -1
  private adapter: NativeRuntimeAdapter = new NativeRuntimeAdapter()
  private pressedVirtualKeys: Set<number> = new Set<number>()
  private autoReleaseTimers: Map<number, number> = new Map<number, number>()

  private static readonly MR_KEY_0: number = 0
  private static readonly MR_KEY_STAR: number = 10
  private static readonly MR_KEY_POUND: number = 11
  private static readonly MR_KEY_UP: number = 12
  private static readonly MR_KEY_DOWN: number = 13
  private static readonly MR_KEY_LEFT: number = 14
  private static readonly MR_KEY_RIGHT: number = 15
  private static readonly MR_KEY_POWER: number = 16
  private static readonly MR_KEY_SOFTLEFT: number = 17
  private static readonly MR_KEY_SOFTRIGHT: number = 18
  private static readonly MR_KEY_SEND: number = 19
  private static readonly MR_KEY_SELECT: number = 20

  private clamp(value: number, min: number, max: number): number {
    if (value < min) {
      return min
    }
    if (value > max) {
      return max
    }
    return value
  }

  private dispatchTouch(action: 'down' | 'move' | 'up', x: number, y: number): void {
    const now = Date.now()
    if (action === 'move') {
      // Coalesce dense move events to avoid input flooding and frame jank.
      if (now - this.lastMoveTs < 16) {
        return
      }
      this.lastMoveTs = now
    } else {
      this.lastMoveTs = now
    }

    const runtimeWidth = this.frameWidth > 0 ? this.frameWidth : 240
    const runtimeHeight = this.frameHeight > 0 ? this.frameHeight : 320
    const safeViewWidth = this.surfaceWidth > 0 ? this.surfaceWidth : runtimeWidth
    const safeViewHeight = this.surfaceHeight > 0 ? this.surfaceHeight : runtimeHeight
    const mappedX = this.clamp(Math.round(x * runtimeWidth / safeViewWidth), 0, runtimeWidth - 1)
    const mappedY = this.clamp(Math.round(y * runtimeHeight / safeViewHeight), 0, runtimeHeight - 1)
    void this.adapter.sendTouch(action, mappedX, mappedY).catch((_error: Error) => {
      // Ignore transient send errors while runtime is switching state.
    })
  }

  private dispatchVirtualKey(action: 'down' | 'up', keyCode: number): void {
    if (this.isDpadDirectionKey(keyCode)) {
      if (action === 'down') {
        this.activeKeyCode = keyCode
        void this.adapter.sendKey('tap', keyCode).catch((_error: Error) => {
          // Ignore transient send errors while runtime is switching state.
        })
        setTimeout(() => {
          if (this.activeKeyCode === keyCode) {
            this.activeKeyCode = -1
          }
        }, 45)
      }
      return
    }

    if (action === 'down') {
      if (this.pressedVirtualKeys.has(keyCode)) {
        return
      }
      this.pressedVirtualKeys.add(keyCode)
      this.activeKeyCode = keyCode
    } else {
      this.clearAutoRelease(keyCode)
      if (!this.pressedVirtualKeys.has(keyCode)) {
        return
      }
      this.pressedVirtualKeys.delete(keyCode)
      if (this.activeKeyCode === keyCode) {
        this.activeKeyCode = -1
      }
    }
    void this.adapter.sendKey(action, keyCode).catch((_error: Error) => {
      // Ignore transient send errors while runtime is switching state.
    })
  }

  private handleVirtualKeyTouch(event: TouchEvent, keyCode: number): void {
    if (event.type === TouchType.Down) {
      this.dispatchVirtualKey('down', keyCode)
    } else if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
      this.dispatchVirtualKey('up', keyCode)
    }
  }

  private isDpadDirectionKey(keyCode: number): boolean {
    return keyCode === RuntimeContainer.MR_KEY_UP ||
      keyCode === RuntimeContainer.MR_KEY_DOWN ||
      keyCode === RuntimeContainer.MR_KEY_LEFT ||
      keyCode === RuntimeContainer.MR_KEY_RIGHT
  }

  private clearAutoRelease(keyCode: number): void {
    const timer = this.autoReleaseTimers.get(keyCode)
    if (timer !== undefined) {
      clearTimeout(timer)
      this.autoReleaseTimers.delete(keyCode)
    }
  }

  private scheduleAutoRelease(keyCode: number): void {
    this.clearAutoRelease(keyCode)
    const timer = setTimeout(() => {
      this.autoReleaseTimers.delete(keyCode)
      this.dispatchVirtualKey('up', keyCode)
    }, 50)
    this.autoReleaseTimers.set(keyCode, timer)
  }

  private frameColor(): string {
    const palette: string[] = ['#DCEBFF', '#C6F6D5', '#FEEBC8', '#FED7E2', '#E9D8FD']
    return this.hasFrame ? palette[this.frameId % palette.length] : '#F3F4F6'
  }

  private keyTextColor(tone: string): string {
    if (tone === 'call') {
      return '#D8FFE6'
    }
    if (tone === 'end') {
      return '#FFE3E0'
    }
    return '#F5FAFE'
  }

  private keyCapImage(keyCode: number, tone: string): Resource {
    if (this.activeKeyCode === keyCode) {
      if (tone === 'call') {
        return $r('app.media.runtime_keycap_call_img')
      }
      if (tone === 'end') {
        return $r('app.media.runtime_keycap_end_img')
      }
      return $r('app.media.runtime_keycap_normal_pressed_img')
    }
    if (tone === 'soft') {
      return $r('app.media.runtime_keycap_soft_img')
    }
    if (tone === 'call') {
      return $r('app.media.runtime_keycap_call_img')
    }
    if (tone === 'end') {
      return $r('app.media.runtime_keycap_end_img')
    }
    if (tone === 'dpad') {
      return keyCode === RuntimeContainer.MR_KEY_SELECT
        ? $r('app.media.runtime_keycap_ok_img')
        : $r('app.media.runtime_keycap_dpad_img')
    }
    return $r('app.media.runtime_keycap_normal_img')
  }

  private phoneShellWidth(): number {
    return Math.round(268 * this.clamp(this.scalePercent, 80, 140) / 100)
  }

  private phoneShellHeight(): number {
    return Math.round(602 * this.clamp(this.scalePercent, 80, 140) / 100)
  }

  private touchSurfaceWidth(): number {
    return Math.round(240 * this.clamp(this.scalePercent, 80, 140) / 100)
  }

  private touchSurfaceHeight(): number {
    return Math.round(this.touchSurfaceWidth() * 4 / 3)
  }

  private shellX(value: number): number {
    return Math.round(value * this.phoneShellWidth() / 615)
  }

  private shellY(value: number): number {
    return Math.round(value * this.phoneShellHeight() / 1381)
  }

  private screenFrameWidth(): number {
    return this.shellX(457)
  }

  private screenFrameHeight(): number {
    return this.shellY(602)
  }

  private keyTextSize(label: string): number {
    return label.length > 3 ? 11 : (label.length > 1 ? 13 : 18)
  }

  private loadingTitle(): string {
    if (this.hint.indexOf('重启') >= 0) {
      return '正在重启'
    }
    if (this.hint.indexOf('启动') >= 0) {
      return '正在启动'
    }
    return '正在载入'
  }

  private loadingStatus(): string {
    if (this.hint.indexOf('首帧') >= 0 || this.hint.indexOf('画面') >= 0) {
      return '等待首帧渲染'
    }
    if (this.hint.length > 0) {
      return this.hint.replace('vMRP', 'MRP')
    }
    return '准备运行环境'
  }

  @Builder
  private LoadingScreen() {
    Stack() {
      Column()
        .width('100%')
        .height('100%')
        .linearGradient({
          angle: 160,
          colors: [['#111B24', 0.0], ['#061018', 0.56], ['#020406', 1.0]]
        })

      Column({ space: 10 }) {
        Row({ space: 4 }) {
          ForEach([0, 1, 2], (item: number) => {
            Rect()
              .width(5)
              .height(item === 1 ? 18 : 12)
              .radius(2)
              .fill(item === 1 ? '#85E7FF' : '#31586B')
              .opacity(item === 2 ? 0.5 : 1)
          })
        }
        .height(24)
        .alignItems(VerticalAlign.Center)

        Text(this.loadingTitle())
          .fontSize(17)
          .fontWeight(FontWeight.Bold)
          .fontColor('#F5FBFF')
          .maxLines(1)
          .textAlign(TextAlign.Center)

        Text(this.loadingStatus())
          .fontSize(11)
          .fontColor('#9EB4C3')
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .textAlign(TextAlign.Center)
          .width(150)

        Row({ space: 5 }) {
          ForEach([0, 1, 2, 3, 4], (item: number) => {
            Column()
              .width(item < 3 ? 22 : 12)
              .height(3)
              .borderRadius(2)
              .backgroundColor(item < 3 ? '#82DDF2' : '#263E4D')
          })
        }
        .margin({ top: 2 })

        Text('240 x 320')
          .fontSize(10)
          .fontColor('#557284')
          .maxLines(1)
      }
      .width(176)
      .padding({ left: 14, right: 14, top: 16, bottom: 16 })
      .borderRadius(8)
      .border({ width: 1, color: '#2C4B5D' })
      .backgroundColor('#121F29D9')
      .alignItems(HorizontalAlign.Center)

      Row() {
        Column()
          .width(30)
          .height(2)
          .backgroundColor('#557284')
          .opacity(0.65)
      }
      .width('100%')
      .justifyContent(FlexAlign.Center)
      .position({ x: 0, y: 18 })
    }
    .width('100%')
    .height('100%')
  }

  @Builder
  private PhoneKey(label: string, keyCode: number, width: number, height: number, tone: string = 'normal') {
    Stack() {
      Image(this.keyCapImage(keyCode, tone))
        .width('100%')
        .height('100%')
        .objectFit(ImageFit.Fill)

      Text(label)
        .fontSize(label.length > 2 ? 11 : (label.length > 1 ? 13 : 18))
        .fontWeight(FontWeight.Medium)
        .fontColor(this.keyTextColor(tone))
        .maxLines(1)
        .textAlign(TextAlign.Center)
        .width('100%')
    }
      .width(width)
      .height(height)
      .opacity(this.activeKeyCode === keyCode ? 0.9 : 1)
      .onTouch((event: TouchEvent) => this.handleVirtualKeyTouch(event, keyCode))
  }

  @Builder
  private DPadControl() {
    Stack() {
      Image($r('app.media.runtime_keypad_dpad_plate_img'))
        .width('100%')
        .height('100%')
        .objectFit(ImageFit.Fill)

      Column({ space: 4 }) {
        this.PhoneKey('▲', RuntimeContainer.MR_KEY_UP, 38, 25, 'dpad')
        Row({ space: 3 }) {
          this.PhoneKey('◀', RuntimeContainer.MR_KEY_LEFT, 38, 29, 'dpad')
          this.PhoneKey('OK', RuntimeContainer.MR_KEY_SELECT, 44, 34, 'dpad')
          this.PhoneKey('▶', RuntimeContainer.MR_KEY_RIGHT, 38, 29, 'dpad')
        }
        this.PhoneKey('▼', RuntimeContainer.MR_KEY_DOWN, 38, 25, 'dpad')
      }
      .alignItems(HorizontalAlign.Center)
      .justifyContent(FlexAlign.Center)
    }
    .width(128)
    .height(98)
  }

  @Builder
  private FrameStatsOverlay() {
    if (this.showFrameStats) {
      Text(`${this.runtimeFps} FPS`)
        .fontSize(10)
        .fontColor('#D8F6FF')
        .fontWeight(FontWeight.Medium)
        .padding({ left: 7, right: 7, top: 4, bottom: 4 })
        .borderRadius(6)
        .backgroundColor('#66000000')
        .position({ x: 5, y: 5 })
    }
  }

  @Builder
  private RuntimeSurface(width: number, height: number, radius: number) {
    Stack() {
      XComponent({ id: 'mrpSurface', type: 'surface', libraryname: 'mrp_napi' })
        .width('100%')
        .height('100%')
      if (!this.hasFrame && !this.firstFrameWaitExpired) {
        this.LoadingScreen()
      }
      this.FrameStatsOverlay()
    }
    .onTouch((event: TouchEvent) => {
      const points = event.changedTouches && event.changedTouches.length > 0 ? event.changedTouches : event.touches
      if (!points || points.length === 0) {
        return
      }
      const p = points[0]
      let action: 'down' | 'move' | 'up' | undefined = undefined
      if (event.type === TouchType.Down) {
        action = 'down'
      } else if (event.type === TouchType.Move) {
        action = 'move'
      } else if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        action = 'up'
      }
      if (!action) {
        return
      }
      this.dispatchTouch(action, Number(p.x ?? 0), Number(p.y ?? 0))
    })
    .width(width)
    .height(height)
    .backgroundColor('#020406')
    .borderRadius(radius)
    .clip(true)
    .onAreaChange((_oldValue: Area, newValue: Area) => {
      this.surfaceWidth = Number(newValue.width)
      this.surfaceHeight = Number(newValue.height)
    })
  }

  @Builder
  private ShellHotKey(
    label: string,
    keyCode: number,
    x: number,
    y: number,
    width: number,
    height: number,
    tone: string = 'normal',
    hitSlopX: number = 18,
    hitSlopY: number = 10
  ) {
    Stack() {
      if (label.length > 0) {
        Row() {
          Text(label)
            .fontSize(this.keyTextSize(label))
            .fontWeight(FontWeight.Medium)
            .fontColor(this.keyTextColor(tone))
            .maxLines(1)
            .textAlign(TextAlign.Center)
            .width('100%')
            .opacity(this.activeKeyCode === keyCode ? 0.7 : 1)
        }
        .width(this.shellX(width))
        .height(this.shellY(height))
        .justifyContent(FlexAlign.Center)
        .alignItems(VerticalAlign.Center)
        .position({ x: this.shellX(hitSlopX), y: this.shellY(hitSlopY) })
      }
    }
    .width(this.shellX(width + hitSlopX * 2))
    .height(this.shellY(height + hitSlopY * 2))
    .position({ x: this.shellX(x - hitSlopX), y: this.shellY(y - hitSlopY) })
    .backgroundColor('#01FFFFFF')
    .onTouch((event: TouchEvent) => this.handleVirtualKeyTouch(event, keyCode))
  }

  @Builder
  private ShellRuntime() {
    Stack() {
        Image($r('app.media.runtime_phone_shell_img'))
          .width(this.phoneShellWidth())
          .height(this.phoneShellHeight())
          .objectFit(ImageFit.Fill)

        Stack() {
          this.RuntimeSurface(this.screenFrameWidth(), this.screenFrameHeight(), 3)
        }
        .width(this.screenFrameWidth())
        .height(this.screenFrameHeight())
        .position({ x: this.shellX(79), y: this.shellY(96) })

        this.ShellHotKey('左软', RuntimeContainer.MR_KEY_SOFTLEFT, 56, 777, 130, 58, 'soft', 16, 10)
        this.ShellHotKey('右软', RuntimeContainer.MR_KEY_SOFTRIGHT, 424, 777, 130, 58, 'soft', 16, 10)
        this.ShellHotKey('', RuntimeContainer.MR_KEY_SEND, 58, 872, 135, 58, 'call', 16, 10)
        this.ShellHotKey('', RuntimeContainer.MR_KEY_POWER, 420, 872, 135, 58, 'end', 16, 10)
        this.ShellHotKey('', RuntimeContainer.MR_KEY_UP, 268, 768, 78, 30, 'dpad', 8, 8)
        this.ShellHotKey('', RuntimeContainer.MR_KEY_LEFT, 206, 824, 44, 80, 'dpad', 8, 8)
        this.ShellHotKey('OK', RuntimeContainer.MR_KEY_SELECT, 274, 832, 64, 64, 'dpad', 12, 12)
        this.ShellHotKey('', RuntimeContainer.MR_KEY_RIGHT, 366, 824, 46, 80, 'dpad', 8, 8)
        this.ShellHotKey('', RuntimeContainer.MR_KEY_DOWN, 268, 924, 78, 30, 'dpad', 8, 8)

        if (this.keypadMode === 'classic') {
          this.ShellHotKey('1', 1, 64, 982, 130, 52)
          this.ShellHotKey('2 ABC', 2, 244, 982, 130, 52)
          this.ShellHotKey('3 DEF', 3, 416, 982, 130, 52)
          this.ShellHotKey('4 GHI', 4, 64, 1060, 130, 52)
          this.ShellHotKey('5 JKL', 5, 244, 1060, 130, 52)
          this.ShellHotKey('6 MNO', 6, 416, 1060, 130, 52)
          this.ShellHotKey('7 PQRS', 7, 64, 1140, 130, 52)
          this.ShellHotKey('8 TUV', 8, 244, 1140, 130, 52)
          this.ShellHotKey('9 WXYZ', 9, 416, 1140, 130, 52)
          this.ShellHotKey('*', RuntimeContainer.MR_KEY_STAR, 64, 1234, 130, 52)
          this.ShellHotKey('0', RuntimeContainer.MR_KEY_0, 244, 1224, 130, 52)
          this.ShellHotKey('#', RuntimeContainer.MR_KEY_POUND, 416, 1224, 130, 52)
        }
      }
      .width(this.phoneShellWidth())
      .height(this.phoneShellHeight())
  }

  @Builder
  private TouchRuntime() {
    Column({ space: 10 }) {
      Stack() {
        this.RuntimeSurface(this.touchSurfaceWidth(), this.touchSurfaceHeight(), 12)
      }
      .width(this.touchSurfaceWidth())
      .height(this.touchSurfaceHeight())
      .borderRadius(12)
      .border({ width: 1, color: '#2C4B5D' })

      Row({ space: 8 }) {
        this.PhoneKey('左软', RuntimeContainer.MR_KEY_SOFTLEFT, 62, 36, 'soft')
        this.PhoneKey('OK', RuntimeContainer.MR_KEY_SELECT, 62, 36, 'dpad')
        this.PhoneKey('右软', RuntimeContainer.MR_KEY_SOFTRIGHT, 62, 36, 'soft')
      }
      .justifyContent(FlexAlign.Center)
    }
    .width('100%')
    .alignItems(HorizontalAlign.Center)
  }

  @Builder
  private CompactDPad() {
    Column({ space: 5 }) {
      this.PhoneKey('上', RuntimeContainer.MR_KEY_UP, 52, 31, 'dpad')
      Row({ space: 5 }) {
        this.PhoneKey('左', RuntimeContainer.MR_KEY_LEFT, 52, 34, 'dpad')
        this.PhoneKey('OK', RuntimeContainer.MR_KEY_SELECT, 58, 38, 'dpad')
        this.PhoneKey('右', RuntimeContainer.MR_KEY_RIGHT, 52, 34, 'dpad')
      }
      .alignItems(VerticalAlign.Center)
      this.PhoneKey('下', RuntimeContainer.MR_KEY_DOWN, 52, 31, 'dpad')
    }
    .alignItems(HorizontalAlign.Center)
  }

  @Builder
  private CompactRuntime() {
    Column({ space: 12 }) {
      Stack() {
        this.RuntimeSurface(this.touchSurfaceWidth(), this.touchSurfaceHeight(), 12)
      }
      .width(this.touchSurfaceWidth())
      .height(this.touchSurfaceHeight())
      .borderRadius(12)
      .border({ width: 1, color: '#2C4B5D' })

      Column({ space: 8 }) {
        Row({ space: 8 }) {
          this.PhoneKey('左软', RuntimeContainer.MR_KEY_SOFTLEFT, 74, 36, 'soft')
          this.PhoneKey('右软', RuntimeContainer.MR_KEY_SOFTRIGHT, 74, 36, 'soft')
        }
        .justifyContent(FlexAlign.Center)

        Column({ space: 8 }) {
          this.CompactDPad()

          Row({ space: 14 }) {
            this.PhoneKey('', RuntimeContainer.MR_KEY_SEND, 58, 34, 'call')
            this.PhoneKey('', RuntimeContainer.MR_KEY_POWER, 58, 34, 'end')
          }
          .justifyContent(FlexAlign.Center)
        }
        .justifyContent(FlexAlign.Center)
        .alignItems(HorizontalAlign.Center)
      }
      .width(this.touchSurfaceWidth())
      .padding({ left: 10, right: 10, top: 10, bottom: 12 })
      .borderRadius(18)
      .linearGradient({
        angle: 160,
        colors: [['#12202AE6', 0.0], ['#071018E6', 1.0]]
      })
      .border({ width: 1, color: '#2C4B5D' })
    }
    .width('100%')
    .alignItems(HorizontalAlign.Center)
  }

  build() {
    Column() {
      if (this.keypadMode === 'touch') {
        this.TouchRuntime()
      } else if (this.keypadMode === 'compact') {
        this.CompactRuntime()
      } else {
        this.ShellRuntime()
      }
    }
    .justifyContent(FlexAlign.Start)
    .alignItems(HorizontalAlign.Center)
    .width('100%')
    .height('100%')
    .layoutWeight(1)
  }
}
