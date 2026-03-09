@Component
export struct RuntimeStatusOverlay {
  @Prop text: string = ''

  build() {
    if (this.text) {
      Text(this.text).padding(16)
    }
  }
}
