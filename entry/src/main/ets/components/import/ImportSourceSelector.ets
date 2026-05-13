@ComponentV2
export struct ImportSourceSelector {
  @Event
  onPick?: () => void

  @Builder
  private FolderIllustration() {
    Image($r('app.media.import_file_action_icon'))
      .width(64)
      .height(64)
      .objectFit(ImageFit.Contain)
  }

  build() {
    Row({ space: 12 }) {
      this.FolderIllustration()

      Column({ space: 5 }) {
        Column({ space: 5 }) {
          Text('从本地选择 .mrp 文件')
            .fontSize(16)
            .fontWeight(FontWeight.Bold)
            .fontColor($r('sys.color.ohos_id_color_text_primary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
          Text('选择本地 MRP 文件进行导入')
            .fontSize(11)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
            .lineHeight(17)
            .maxLines(2)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
        }
        .width('100%')
        .alignItems(HorizontalAlign.Start)
      }
      .layoutWeight(1)
      .alignItems(HorizontalAlign.Start)
      .constraintSize({ minWidth: 0 })

      Text('选择')
        .fontSize(13)
        .fontWeight(FontWeight.Medium)
        .fontColor('#1261CF')
        .height(30)
        .padding({ left: 12, right: 12 })
        .borderRadius(15)
        .backgroundColor($r('app.color.app_surface_muted'))
        .border({ width: 1, color: $r('app.color.app_surface_border') })
    }
      .width('100%')
      .height(90)
      .padding({ left: 16, right: 16, top: 14, bottom: 14 })
      .alignItems(VerticalAlign.Center)
      .linearGradient({
        angle: 135,
        colors: [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_primary_soft'), 0.55], [$r('app.color.app_surface_muted'), 1.0]]
      })
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .borderRadius(22)
      .shadow({
        radius: 16,
        color: '#18384C1C',
        offsetX: 0,
        offsetY: 7
      })
      .margin({ top: 0, bottom: 10 })
      .onClick(() => {
        if (this.onPick) {
          this.onPick()
        }
      })
  }
}
