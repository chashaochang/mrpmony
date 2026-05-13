@ComponentV2
export struct PageHeader {
  @Param title: string = ''
  @Param subtitle: string = ''

  build() {
    Row() {
      Column({ space: 4 }) {
        Text(this.title)
          .fontSize(24)
          .fontWeight(FontWeight.Bold)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
        if (this.subtitle.length > 0) {
          Text(this.subtitle)
            .fontSize(13)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
        }
      }
      .alignItems(HorizontalAlign.Start)
    }
    .width('100%')
    .padding({ left: 18, right: 18, top: 16, bottom: 14 })
    .linearGradient({
      angle: 110,
      colors: [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_primary_soft'), 0.62], [$r('app.color.app_surface_muted'), 1.0]]
    })
    .border({
      width: 1,
      color: $r('app.color.app_surface_border')
    })
    .borderRadius(22)
    .shadow({
      radius: 18,
      color: '#2A5D8620',
      offsetX: 0,
      offsetY: 8
    })
    .margin({ left: 14, right: 14, top: 10, bottom: 8 })
  }
}
