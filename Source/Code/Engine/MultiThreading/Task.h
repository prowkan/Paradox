#pragma once

class Task
{
	public:

		Task() { FinishFlag.store(false, memory_order::memory_order_seq_cst); }

		virtual void Execute(const UINT ThreadID) = 0;

		void WaitForFinish() { while (!FinishFlag.load(memory_order::memory_order_seq_cst)); }

	protected:

		void Finish() { FinishFlag.store(true, memory_order::memory_order_seq_cst); }

	private:

		atomic<bool> FinishFlag;
};