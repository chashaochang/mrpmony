@Component
export struct PageHeader {
  @Prop title: string = ''

  build() {
    Row() {
      Text(this.title).fontSize(20).fontWeight(FontWeight.Medium)
    }
    .width('100%')
    .padding(16)
  }
}
