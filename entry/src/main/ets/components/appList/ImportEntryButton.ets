@Component
export struct ImportEntryButton {
  onTap?: () => void

  build() {
    Button('导入应用')
      .onClick(() => {
        if (this.onTap) {
          this.onTap()
        }
      })
  }
}
