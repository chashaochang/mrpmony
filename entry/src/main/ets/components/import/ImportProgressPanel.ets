@ComponentV2
export struct ImportProgressPanel {
  @Param text: string = '导入中'

  build() {
    Row({ space: 14 }) {
      LoadingProgress()
        .width(42)
        .height(42)
      Column({ space: 4 }) {
        Text(this.text)
          .fontSize(16)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
        Text('正在复制到模拟器存储')
          .fontSize(12)
          .fontColor($r('sys.color.ohos_id_color_text_secondary'))
      }
      .layoutWeight(1)
      .alignItems(HorizontalAlign.Start)
    }
    .width('100%')
    .padding({ left: 16, right: 16, top: 16, bottom: 16 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor($r('app.color.app_surface'))
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .borderRadius(20)
    .margin({ left: 16, right: 16, top: 10, bottom: 10 })
  }
}
