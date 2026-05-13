@ComponentV2
export struct RuntimeStatusOverlay {
  @Param text: string = ''

  build() {
    if (this.text) {
      Text(this.text).padding(16)
    }
  }
}
