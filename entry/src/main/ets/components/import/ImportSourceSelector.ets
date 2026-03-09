@Component
export struct ImportSourceSelector {
  onPick?: () => void

  build() {
    Button('选择本地文件')
      .onClick(() => {
        if (this.onPick) {
          this.onPick()
        }
      })
  }
}
