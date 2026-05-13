@ComponentV2
export struct ImportResultPanel {
  @Param text: string = ''
  @Param tone: string = 'success'

  build() {
    Row({ space: 8 }) {
      Text(this.tone === 'success' ? '✓' : '!')
        .fontSize(13)
        .fontWeight(FontWeight.Bold)
        .fontColor(this.tone === 'success' ? $r('app.color.app_success_text') : $r('app.color.app_danger_text'))
        .textAlign(TextAlign.Center)
        .width(24)
        .height(24)
        .borderRadius(12)
        .backgroundColor($r('app.color.app_surface_muted'))

      Text(this.text)
        .fontSize(14)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('sys.color.ohos_id_color_text_primary'))
        .layoutWeight(1)
    }
    .height(48)
    .width('100%')
    .padding({ left: 14, right: 14 })
    .alignItems(VerticalAlign.Center)
    .borderRadius(16)
    .backgroundColor(this.tone === 'success' ? $r('app.color.app_success_soft') : $r('app.color.app_danger_soft'))
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .margin({ left: 14, right: 14, top: 8, bottom: 8 })
  }
}
