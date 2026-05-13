@ComponentV2
export struct RuntimeToolbar {
  @Event
  onBack?: () => void
  @Event
  onRestart?: () => void
  @Event
  onPullFrame?: () => void
  @Event
  onExit?: () => void
  @Param actionLabel: string = '重启'
  @Param canRestart: boolean = true
  @Param canPullFrame: boolean = true
  @Param canExit: boolean = true
  @Param packageTitle: string = 'MRP'
  @Param statusText: string = ''

  @Builder
  private BackButton() {
    Button('返回')
      .height(34)
      .fontSize(13)
      .fontWeight(FontWeight.Medium)
      .fontColor($r('sys.color.ohos_id_color_text_primary'))
      .backgroundColor($r('sys.color.ohos_id_color_button_normal'))
      .borderRadius(17)
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .onClick(() => {
        if (this.onBack) {
          this.onBack()
        }
      })
  }

  @Builder
  private ActionButton(label: string, enabled: boolean, action: () => void) {
    Button(label)
      .enabled(enabled)
      .height(32)
      .fontSize(12)
      .fontWeight(FontWeight.Medium)
      .fontColor(enabled ? $r('sys.color.ohos_id_color_text_primary') : $r('sys.color.ohos_id_color_text_hint'))
      .backgroundColor(enabled ? $r('app.color.app_surface_muted') : $r('sys.color.ohos_id_color_button_normal'))
      .borderRadius(16)
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .onClick(action)
  }

  build() {
    Row({ space: 10 }) {
      this.BackButton()

      Column({ space: 2 }) {
        Text(this.packageTitle)
          .fontSize(15)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .width('100%')
        if (this.statusText.length > 0) {
          Text(this.statusText)
            .fontSize(11)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')
        }
      }
      .layoutWeight(1)
      .constraintSize({ minWidth: 0 })
      .alignItems(HorizontalAlign.Start)

      Row({ space: 6 }) {
        this.ActionButton(this.actionLabel, this.canRestart, () => {
          if (this.onRestart) {
            this.onRestart()
          }
        })
        this.ActionButton('刷新', this.canPullFrame, () => {
          if (this.onPullFrame) {
            this.onPullFrame()
          }
        })
        this.ActionButton('退出', this.canExit, () => {
          if (this.onExit) {
            this.onExit()
          }
        })
      }
    }
    .width('100%')
    .height(58)
    .alignItems(VerticalAlign.Center)
    .padding({ left: 8, right: 8, top: 8, bottom: 8 })
    .linearGradient({
      angle: 115,
      colors: [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_primary_soft'), 0.58], [$r('app.color.app_surface_muted'), 1.0]]
    })
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .borderRadius(24)
    .shadow({
      radius: 20,
      color: '#30203045',
      offsetX: 0,
      offsetY: 8
    })
    .margin({ left: 14, right: 14, top: 8, bottom: 8 })
  }
}
