import { AppPreferences, defaultEmulatorSettings } from '../services/AppPreferences'
import type { EmulatorSettings, KeypadMode } from '../services/AppPreferences'

@ComponentV2
export struct SettingsPage {
  @Param scroller: Scroller = new Scroller()
  @Param appId?: string = undefined
  @Param appTitle: string = ''
  @Param onNavigateAbout?: () => void = undefined
  @Event
  onSettingsChanged?: () => void
  @Local keypadMode: KeypadMode = 'classic'
  @Local scalePercent: number = 100
  @Local showFrameStats: boolean = false
  private appPreferences: AppPreferences = new AppPreferences()

  aboutToAppear(): void {
    const settings = this.currentSettings()
    this.keypadMode = settings.keypadMode
    this.scalePercent = settings.scalePercent
    this.showFrameStats = settings.showFrameStats
  }

  private isGameScoped(): boolean {
    return this.appId !== undefined && this.appId.length > 0
  }

  private currentSettings(): EmulatorSettings {
    return this.isGameScoped()
      ? this.appPreferences.emulatorSettingsForApp(this.appId)
      : this.appPreferences.emulatorSettings()
  }

  private saveSettings(): void {
    const settings: EmulatorSettings = {
      keypadMode: this.keypadMode,
      scalePercent: this.scalePercent,
      showFrameStats: this.showFrameStats
    }
    if (this.isGameScoped()) {
      this.appPreferences.saveEmulatorSettingsForApp(this.appId, settings)
    } else {
      this.appPreferences.saveEmulatorSettings(settings)
    }
    if (this.onSettingsChanged) {
      this.onSettingsChanged()
    }
  }

  private applySettings(settings: EmulatorSettings): void {
    this.keypadMode = settings.keypadMode
    this.scalePercent = settings.scalePercent
    this.showFrameStats = settings.showFrameStats
  }

  private resetSettings(): void {
    if (this.isGameScoped()) {
      this.appPreferences.resetEmulatorSettingsForApp(this.appId)
      this.applySettings(this.appPreferences.emulatorSettings())
    } else {
      const settings = defaultEmulatorSettings()
      this.applySettings(settings)
      this.appPreferences.resetEmulatorSettings()
    }
    if (this.onSettingsChanged) {
      this.onSettingsChanged()
    }
  }

  private keypadModeText(): string {
    if (this.keypadMode === 'compact') {
      return '紧凑'
    }
    if (this.keypadMode === 'touch') {
      return '触控'
    }
    return '经典'
  }

  private selectKeypadMode(mode: KeypadMode): void {
    if (this.keypadMode === mode) {
      return
    }
    this.keypadMode = mode
    this.saveSettings()
  }

  private pageTitle(): string {
    return this.isGameScoped() ? '当前游戏设置' : '模拟器设置'
  }

  private resetText(): string {
    return this.isGameScoped() ? '恢复全局' : '恢复默认'
  }

  private openAbout(): void {
    if (this.onNavigateAbout) {
      this.onNavigateAbout()
    }
  }

  @Builder
  private TopSafeGapItem() {
    ListItem() {
      Column()
        .width('100%')
        .height(68)
    }
    .selectable(false)
  }

  @Builder
  private Header() {
    Row({ space: 12 }) {
      Column({ space: 5 }) {
        Text(this.pageTitle())
          .fontSize(26)
          .fontWeight(FontWeight.Bold)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)

      Text(this.resetText())
        .fontSize(13)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('app.color.app_accent_text'))
        .height(36)
        .padding({ left: 13, right: 13 })
        .borderRadius(18)
        .backgroundColor($r('app.color.app_surface_muted'))
        .border({ width: 1, color: $r('app.color.app_surface_border') })
        .onClick(() => this.resetSettings())
    }
    .width('100%')
    .alignItems(VerticalAlign.Center)
    .padding({ left: 16, right: 16, top: 18, bottom: 8 })
  }

  @Builder
  private SectionTitle(title: string) {
    Row() {
      Column() {
        Text(title)
          .fontSize(18)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
      }
      .alignItems(HorizontalAlign.Start)
      .layoutWeight(1)
    }
    .width('100%')
    .padding({ top: 18, bottom: 8 })
  }

  @Builder
  private KeypadModeChip(label: string, mode: KeypadMode) {
    Text(label)
      .fontSize(13)
      .fontWeight(this.keypadMode === mode ? FontWeight.Medium : FontWeight.Regular)
      .fontColor(this.keypadMode === mode ? $r('sys.color.ohos_id_color_text_primary') : $r('sys.color.ohos_id_color_text_secondary'))
      .textAlign(TextAlign.Center)
      .layoutWeight(1)
      .height(38)
      .borderRadius(19)
      .backgroundColor(this.keypadMode === mode ? $r('app.color.app_surface_muted') : $r('sys.color.ohos_id_color_titlebar_bg_transparent'))
      .shadow(this.keypadMode === mode ? {
        radius: 10,
        color: '#18143247',
        offsetX: 0,
        offsetY: 4
      } : {
        radius: 0,
        color: '#00000000',
        offsetX: 0,
        offsetY: 0
      })
      .onClick(() => this.selectKeypadMode(mode))
  }

  @Builder
  private KeypadModeSegmentedSetting() {
    Row({ space: 4 }) {
      this.KeypadModeChip('经典', 'classic')
      this.KeypadModeChip('紧凑', 'compact')
      this.KeypadModeChip('触控', 'touch')
    }
    .width('100%')
    .padding(4)
    .borderRadius(24)
    .linearGradient({
      angle: 120,
      colors: [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_primary_soft'), 0.62], [$r('app.color.app_surface_muted'), 1.0]]
    })
    .border({ width: 1, color: $r('app.color.app_surface_border') })
  }

  @Builder
  private SwitchRow(title: string, enabled: boolean, onToggle: (value: boolean) => void) {
    Row({ space: 12 }) {
      Column() {
        Text(title)
          .fontSize(16)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
      }
      .layoutWeight(1)
      .alignItems(HorizontalAlign.Start)

      Toggle({ type: ToggleType.Switch, isOn: enabled })
        .onChange((value: boolean) => onToggle(value))
    }
    .width('100%')
    .padding({ left: 14, right: 14, top: 12, bottom: 12 })
    .alignItems(VerticalAlign.Center)
    .borderRadius(18)
    .backgroundColor($r('app.color.app_surface_muted'))
    .border({ width: 1, color: $r('app.color.app_surface_border') })
  }

  @Builder
  private LinkRow(title: string, actionLabel: string, onClick: () => void) {
    Row({ space: 12 }) {
      Column() {
        Text(title)
          .fontSize(16)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
      }
      .layoutWeight(1)
      .alignItems(HorizontalAlign.Start)

      Text(actionLabel)
        .fontSize(13)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('app.color.app_accent_text'))
        .width(44)
        .textAlign(TextAlign.End)
    }
    .width('100%')
    .padding({ left: 14, right: 14, top: 14, bottom: 14 })
    .alignItems(VerticalAlign.Center)
    .borderRadius(18)
    .backgroundColor($r('app.color.app_surface_muted'))
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .onClick(() => onClick())
  }

  @Builder
  private HeaderItem() {
    ListItem() {
      this.Header()
    }
    .selectable(false)
  }

  @Builder
  private ScaleSectionItem() {
    ListItem() {
      Column({ space: 10 }) {
        this.SectionTitle('画面缩放')
        Row({ space: 12 }) {
          Text('缩放')
            .fontSize(16)
            .fontWeight(FontWeight.Medium)
            .fontColor($r('sys.color.ohos_id_color_text_primary'))
          Slider({ value: this.scalePercent, min: 80, max: 140, step: 5 })
            .layoutWeight(1)
            .onChange((value: number, _mode: SliderChangeMode) => {
              this.scalePercent = Math.round(value)
              this.saveSettings()
            })
          Text(`${this.scalePercent}%`)
            .fontSize(13)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
            .width(46)
            .textAlign(TextAlign.End)
        }
        .width('100%')
        .padding({ left: 14, right: 14, top: 10, bottom: 12 })
        .borderRadius(18)
        .backgroundColor($r('app.color.app_surface_muted'))
        .border({ width: 1, color: $r('app.color.app_surface_border') })
      }
      .width('100%')
      .padding({ left: 16, right: 16, top: 4, bottom: 8 })
    }
    .selectable(false)
  }

  @Builder
  private KeypadSectionItem() {
    ListItem() {
      Column({ space: 10 }) {
        this.SectionTitle('按键')
        this.KeypadModeSegmentedSetting()
      }
      .width('100%')
      .padding({ left: 16, right: 16, top: 4, bottom: 8 })
    }
    .selectable(false)
  }

  @Builder
  private PerformanceSectionItem() {
    ListItem() {
      Column({ space: 10 }) {
        this.SectionTitle('性能')
        this.SwitchRow('实时 FPS', this.showFrameStats, (value: boolean) => {
          this.showFrameStats = value
          this.saveSettings()
        })
      }
      .width('100%')
      .padding({ left: 16, right: 16, top: 4, bottom: 18 })
    }
    .selectable(false)
  }

  @Builder
  private AboutSectionItem() {
    ListItem() {
      Column({ space: 10 }) {
        this.SectionTitle('关于')
        this.LinkRow('关于 mrpmony', '查看', () => this.openAbout())
      }
      .width('100%')
      .padding({ left: 16, right: 16, top: 4, bottom: 18 })
    }
    .selectable(false)
  }

  @Builder
  private BottomSafeTail() {
    ListItem() {
      Column()
        .width('100%')
        .height(116)
    }
    .selectable(false)
  }

  build() {
    List({ scroller: this.scroller }) {
      ListItem() {
        Text(`${this.keypadMode}-${this.scalePercent}-${this.showFrameStats}`)
          .fontSize(1)
          .opacity(0)
      }
      .height(1)
      .selectable(false)
      this.TopSafeGapItem()
      this.HeaderItem()
      this.ScaleSectionItem()
      this.KeypadSectionItem()
      this.PerformanceSectionItem()
      if (!this.isGameScoped()) {
        this.AboutSectionItem()
      }
      this.BottomSafeTail()
    }
    .width('100%')
    .height('100%')
    .edgeEffect(EdgeEffect.Spring)
    .linearGradient({
      angle: 180,
      colors: [
        [$r('app.color.home_background_start'), 0.0],
        [$r('app.color.home_background_mid'), 0.48],
        [$r('app.color.home_background_end'), 1.0]
      ]
    })
    .expandSafeArea([SafeAreaType.SYSTEM], [SafeAreaEdge.TOP, SafeAreaEdge.BOTTOM])
  }
}
