@Component
export struct RuntimeContainer {
  @Prop hint: string = 'Runtime Surface Placeholder'

  build() {
    Column({ space: 12 }) {
      Text('MRP Runtime Surface Placeholder')
        .fontSize(18)
        .fontWeight(FontWeight.Medium)
      Text(this.hint)
        .fontSize(14)
        .fontColor('#666666')
    }
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .width('100%')
    .layoutWeight(1)
  }
}
