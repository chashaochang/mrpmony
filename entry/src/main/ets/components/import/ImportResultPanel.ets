@Component
export struct ImportResultPanel {
  @Prop text: string = ''

  build() {
    Text(this.text).padding(16)
  }
}
