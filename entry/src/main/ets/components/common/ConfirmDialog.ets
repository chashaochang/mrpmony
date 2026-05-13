@ComponentV2
export struct ConfirmDialog {
  @Param title: string = '确认'
  @Param content: string = ''

  build() {
    Column() {
      Text(this.title)
      Text(this.content).margin({ top: 8 })
    }
    .padding(16)
  }
}
