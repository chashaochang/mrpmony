@ComponentV2
export struct LoadingBlock {
  @Param text: string = '加载中'

  build() {
    Row({ space: 8 }) {
      LoadingProgress()
        .width(18)
        .height(18)
      Text(this.text)
        .fontSize(12)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('sys.color.ohos_id_color_text_secondary'))
    }
    .justifyContent(FlexAlign.Center)
    .alignItems(VerticalAlign.Center)
    .alignSelf(ItemAlign.Center)
    .padding({ left: 14, right: 14, top: 9, bottom: 9 })
    .margin({ top: 8, bottom: 8 })
    .borderRadius(16)
    .backgroundColor($r('app.color.app_surface'))
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .shadow({
      radius: 18,
      color: '#14000000',
      offsetX: 0,
      offsetY: 8
    })
  }
}
