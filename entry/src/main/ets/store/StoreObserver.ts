export type StoreListener = () => void

export class StoreObserver {
  private listeners: Set<StoreListener> = new Set()

  subscribe(listener: StoreListener): () => void {
    this.listeners.add(listener)
    return () => {
      this.listeners.delete(listener)
    }
  }

  protected notify(): void {
    this.listeners.forEach((listener) => listener())
  }
}
