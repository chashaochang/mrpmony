import worker, { ErrorEvent, MessageEvents, ThreadWorkerPriority } from '@ohos.worker'

interface RecentWorkerRequest {
  action: string
  appId?: string
  appIdsText?: string
  currentText: string
}

interface RecentWorkerResponse {
  action: string
  recentAppIdsText: string
}

export class RecentRunRecorder {
  private threadWorker?: worker.ThreadWorker
  private currentText: string = ''
  private onChanged?: (recentAppIdsText: string) => void

  configure(initialText: string, onChanged: (recentAppIdsText: string) => void): void {
    this.currentText = initialText
    this.onChanged = onChanged
  }

  updateCurrentText(text: string): void {
    this.currentText = text
  }

  record(appId: string): void {
    this.post({
      action: 'record',
      appId,
      currentText: this.currentText
    })
  }

  clear(): void {
    this.post({
      action: 'clear',
      currentText: this.currentText
    })
  }

  remove(appId: string): void {
    this.post({
      action: 'remove',
      appId,
      currentText: this.currentText
    })
  }

  removeMany(appIdsText: string): void {
    this.post({
      action: 'removeMany',
      appIdsText,
      currentText: this.currentText
    })
  }

  private ensureWorker(): worker.ThreadWorker | undefined {
    if (this.threadWorker) {
      return this.threadWorker
    }
    try {
      this.threadWorker = new worker.ThreadWorker('entry/ets/workers/RecentRunWorker.ets', {
        name: 'recent-run-recorder',
        priority: ThreadWorkerPriority.IDLE
      })
      this.threadWorker.onmessage = (event: MessageEvents) => {
        const response = event.data as RecentWorkerResponse
        if (response.action !== 'recentChanged') {
          return
        }
        this.currentText = response.recentAppIdsText
        if (this.onChanged) {
          this.onChanged(response.recentAppIdsText)
        }
      }
      this.threadWorker.onerror = (_error: ErrorEvent) => {
        this.threadWorker = undefined
      }
      this.threadWorker.onexit = (_code: number) => {
        this.threadWorker = undefined
      }
      return this.threadWorker
    } catch (_error) {
      this.threadWorker = undefined
      return undefined
    }
  }

  private post(request: RecentWorkerRequest): void {
    const threadWorker = this.ensureWorker()
    if (!threadWorker) {
      return
    }
    try {
      threadWorker.postMessage(request)
    } catch (_error) {
      this.threadWorker = undefined
    }
  }
}
