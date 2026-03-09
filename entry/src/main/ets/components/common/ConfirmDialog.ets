@Component
export struct ConfirmDialog {
  @Prop title: string = '确认'
  @Prop content: string = ''

  build() {
    Column() {
      Text(this.title)
      Text(this.content).margin({ top: 8 })
    }
    .padding(16)
  }
}
