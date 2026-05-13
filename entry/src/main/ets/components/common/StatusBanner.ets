@ComponentV2
export struct StatusBanner {
  @Param text: string = ''
  @Param tone: string = 'info'

  private iconText(): string {
    if (this.tone === 'error') {
      return '!'
    }
    if (this.tone === 'success') {
      return '✓'
    }
    return 'i'
  }

  private iconColor(): Resource {
    if (this.tone === 'error') {
      return $r('app.color.app_danger_text')
    }
    if (this.tone === 'success') {
      return $r('app.color.app_success_text')
    }
    return $r('app.color.app_accent_text')
  }

  private textColor(): Resource {
    if (this.tone === 'error') {
      return $r('app.color.app_danger_text')
    }
    return $r('sys.color.ohos_id_color_text_primary')
  }

  build() {
    if (this.text) {
      Row({ space: 8 }) {
        Text(this.iconText())
          .fontSize(12)
          .fontWeight(FontWeight.Bold)
          .fontColor(this.iconColor())
          .textAlign(TextAlign.Center)
          .width(22)
          .height(22)
          .borderRadius(11)
          .backgroundColor($r('app.color.app_surface_muted'))
        Text(this.text)
          .fontSize(13)
          .lineHeight(19)
          .fontColor(this.textColor())
          .layoutWeight(1)
          .maxLines(3)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }
        .width('100%')
        .alignItems(VerticalAlign.Center)
        .padding({ left: 14, right: 14, top: 12, bottom: 12 })
        .backgroundColor($r('app.color.app_surface'))
        .border({
          width: 1,
          color: $r('app.color.app_surface_border')
        })
        .borderRadius(16)
        .shadow({
          radius: 14,
          color: '#18384C1F',
          offsetX: 0,
          offsetY: 5
        })
        .margin({ left: 14, right: 14, top: 4, bottom: 10 })
    }
  }
}
