@ComponentV2
export struct ImportEntryButton {
  @Event
  onTap?: () => void

  build() {
    Row({ space: 8 }) {
      Text('+')
        .fontSize(24)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('app.color.app_accent_text'))
        .textAlign(TextAlign.Center)
        .width(21)
        .height(24)
      Text('导入 MRP')
        .fontSize(16)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('sys.color.ohos_id_color_text_primary'))
    }
      .justifyContent(FlexAlign.Center)
      .alignItems(VerticalAlign.Center)
      .height(48)
      .width('100%')
      .backgroundColor($r('app.color.app_primary_soft'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .borderRadius(14)
      .shadow({
        radius: 16,
        color: '#28123A4B',
        offsetX: 0,
        offsetY: 7
      })
      .margin({ left: 14, right: 14, top: 8, bottom: 14 })
      .onClick(() => {
        if (this.onTap) {
          this.onTap()
        }
      })
  }
}
