@ComponentV2
export struct PackagePreviewCard {
  @Param name: string = ''
  @Param valid: boolean = false

  @Builder
  private FileIcon() {
    Stack() {
      Column()
        .width(40)
        .height(46)
        .borderRadius(9)
        .linearGradient({
          angle: 140,
          colors: this.valid
            ? [['#A7D7FF', 0.0], ['#2F84EA', 1.0]]
            : [[$r('app.color.app_danger_soft'), 0.0], [$r('app.color.app_surface_muted'), 1.0]]
        })
        .border({ width: 1, color: this.valid ? '#D8FFFFFF' : $r('app.color.app_surface_border') })
        .shadow({
          radius: 10,
          color: this.valid ? '#2F84EA33' : '#00000000',
          offsetX: 0,
          offsetY: 5
        })

      Column()
        .width(12)
        .height(12)
        .borderRadius(4)
        .backgroundColor('#E9F5FF')
        .translate({ x: 14, y: -17 })
        .opacity(this.valid ? 0.95 : 0.5)

      Text('MRP')
        .fontSize(12)
        .fontWeight(FontWeight.Bold)
        .fontColor(this.valid ? '#FFFFFF' : $r('app.color.app_danger_text'))
        .translate({ y: 6 })
    }
    .width(48)
    .height(52)
  }

  @Builder
  private StateMark() {
    Column() {
      Text(this.valid ? '✓' : '!')
        .fontSize(15)
        .fontWeight(FontWeight.Bold)
        .fontColor('#FFFFFF')
        .textAlign(TextAlign.Center)
    }
    .width(28)
    .height(28)
    .borderRadius(14)
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .backgroundColor(this.valid ? '#236DDE' : $r('app.color.app_danger_text'))
  }

  build() {
    Row({ space: 12 }) {
      this.FileIcon()

      Column({ space: 4 }) {
        Text(this.name || '未选择文件')
          .fontSize(15)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .width('100%')
      }
      .layoutWeight(1)
      .alignItems(HorizontalAlign.Start)
      .constraintSize({ minWidth: 0 })

      this.StateMark()
    }
    .height(66)
    .padding({ left: 10, right: 10, top: 7, bottom: 7 })
    .alignItems(VerticalAlign.Center)
    .width('100%')
    .borderRadius(16)
    .backgroundColor($r('app.color.app_surface_muted'))
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .margin({ bottom: 0 })
  }
}
