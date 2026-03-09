@Component
export struct RuntimeContainer {
  @Prop hint: string = 'Runtime Surface Placeholder'
  @Prop hasFrame: boolean = false
  @Prop frameId: number = 0
  @Prop frameWidth: number = 0
  @Prop frameHeight: number = 0

  private frameColor(): string {
    const palette: string[] = ['#DCEBFF', '#C6F6D5', '#FEEBC8', '#FED7E2', '#E9D8FD']
    return this.hasFrame ? palette[this.frameId % palette.length] : '#F3F4F6'
  }

  build() {
    Column({ space: 12 }) {
      Text('MRP Runtime Surface Placeholder')
        .fontSize(18)
        .fontWeight(FontWeight.Medium)
      Stack() {
        Column() {
          Text(this.hasFrame ? `测试帧 #${this.frameId}` : '尚未拉到测试帧')
            .fontSize(18)
            .fontWeight(FontWeight.Bold)
          Text(this.hasFrame ? `${this.frameWidth} x ${this.frameHeight}` : '等待 pullFrame()')
            .fontSize(14)
            .fontColor('#666666')
        }
      }
      .width('88%')
      .aspectRatio(3 / 4)
      .backgroundColor(this.frameColor())
      .borderRadius(16)
      .padding(16)
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
