@Component
export struct StatusBanner {
  @Prop text: string = ''

  build() {
    if (this.text) {
      Text(this.text)
        .width('100%')
        .padding(12)
        .backgroundColor('#FFF3CD')
    }
  }
}
