@ComponentV2
export struct EmptyState {
  @Param text: string = '暂无数据'

  build() {
    Column({ space: 8 }) {
      Text(this.text)
        .fontSize(15)
        .fontColor($r('sys.color.ohos_id_color_text_primary'))
      Text('导入本地 .mrp 文件后会显示在这里')
        .fontSize(12)
        .fontColor($r('sys.color.ohos_id_color_text_secondary'))
    }
    .width('100%')
    .alignItems(HorizontalAlign.Center)
    .padding({ left: 20, right: 20, top: 28, bottom: 28 })
    .margin({ left: 14, right: 14, top: 12, bottom: 8 })
    .borderRadius(20)
    .linearGradient({
      angle: 120,
      colors: [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_surface_muted'), 1.0]]
    })
    .border({ width: 1, color: $r('app.color.app_surface_border') })
  }
}
