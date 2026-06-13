namespace pawn_natives
{
	template <typename RET, PAWN_HOOK_TEMPLATE>
	class PAWN_HOOK_NAME : protected NativeHookBase
	{
	public:
		typedef RET (*implementation_t)(PAWN_HOOK_TYPES);

		class ScopedCall
		{
		public:
			inline RET operator()(PAWN_HOOK_PARAMETERS)
			{
				return original_(PAWN_HOOK_CALLING);
			}

			~ScopedCall()
			{
				if (removed_)
					hook_.Install();
			}

		private:
			ScopedCall(subhook::Hook & hook, implementation_t original)
			:
				hook_(hook),
				original_(original),
				removed_(hook.Remove())
			{
			}

			ScopedCall(ScopedCall const &) = default;
			ScopedCall & operator=(ScopedCall const &) = default;

			ScopedCall(ScopedCall && that)
			:
				hook_(that.hook_),
				original_(that.original_),
				removed_(that.removed_)
			{
				that.removed_ = false;
			}

			ScopedCall & operator=(ScopedCall const &&) = delete;

			ScopedCall() = delete;

			friend class PAWN_HOOK_NAME<RET, PAWN_HOOK_TYPES>;

			subhook::Hook &
				hook_;

			implementation_t const
				original_;

			bool
				removed_;
		};

		inline RET operator()(PAWN_HOOK_PARAMETERS)
		{
			RET
				ret;
			if (Recursing())
			{
				subhook::ScopedHookRemove
					undo(&GetHook());
				ret = original_(PAWN_HOOK_CALLING);
			}
			else
			{
				ret = Do(PAWN_HOOK_CALLING);
			}
			Recursing();
			return ret;
		}

		ScopedCall operator*() const
		{
			ScopedCall
				ret(GetHook(), original_);
			return ret;
		}

	protected:
		PAWN_HOOK_NAME(char const * const name, implementation_t original, AMX_NATIVE replacement) : NativeHookBase(PAWN_HOOK_NUMBER, name, replacement), original_(original) {}
		~PAWN_HOOK_NAME() = default;

	private:
		cell CallDoInner(AMX * amx, cell * params)
		{
				int idx = 1;
			#if PAWN_HOOK_NUMBER >= 1
				ParamCast<A> a(amx, params, idx);
				idx += ParamCast<A>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 2
				ParamCast<B> b(amx, params, idx);
				idx += ParamCast<B>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 3
				ParamCast<C> c(amx, params, idx);
				idx += ParamCast<C>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 4
				ParamCast<D> d(amx, params, idx);
				idx += ParamCast<D>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 5
				ParamCast<E> e(amx, params, idx);
				idx += ParamCast<E>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 6
				ParamCast<F> f(amx, params, idx);
				idx += ParamCast<F>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 7
				ParamCast<G> g(amx, params, idx);
				idx += ParamCast<G>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 8
				ParamCast<H> h(amx, params, idx);
				idx += ParamCast<H>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 9
				ParamCast<I> i(amx, params, idx);
				idx += ParamCast<I>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 10
				ParamCast<J> j(amx, params, idx);
				idx += ParamCast<J>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 11
				ParamCast<K> k(amx, params, idx);
				idx += ParamCast<K>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 12
				ParamCast<L> l(amx, params, idx);
				idx += ParamCast<L>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 13
				ParamCast<M> m(amx, params, idx);
				idx += ParamCast<M>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 14
				ParamCast<N> n(amx, params, idx);
				idx += ParamCast<N>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 15
				ParamCast<O> o(amx, params, idx);
				idx += ParamCast<O>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 16
				ParamCast<P> p(amx, params, idx);
				idx += ParamCast<P>::Size;
			#endif
				RET
					ret = this->Do(PAWN_HOOK_CALLING);
				return *(cell *)&ret;
		}

		virtual RET Do(PAWN_HOOK_PARAMETERS) const = 0;

		implementation_t const
			original_;
	};

	template <PAWN_HOOK_TEMPLATE>
	class PAWN_HOOK_NAME<void, PAWN_HOOK_TYPES> : protected NativeHookBase
	{
	public:
		typedef void (*implementation_t)(PAWN_HOOK_TYPES);

		class ScopedCall
		{
		public:
			inline void operator()(PAWN_HOOK_PARAMETERS)
			{
				original_(PAWN_HOOK_CALLING);
			}

			~ScopedCall()
			{
				if (removed_)
					hook_.Install();
			}

		private:
			ScopedCall(subhook::Hook & hook, implementation_t original)
			:
				hook_(hook),
				original_(original),
				removed_(hook.Remove())
			{
			}

			ScopedCall(ScopedCall const &) = default;
			ScopedCall & operator=(ScopedCall const &) = default;

			ScopedCall(ScopedCall && that)
			:
				hook_(that.hook_),
				original_(that.original_),
				removed_(that.removed_)
			{
				that.removed_ = false;
			}

			ScopedCall & operator=(ScopedCall const &&) = delete;

			ScopedCall() = delete;

			friend class PAWN_HOOK_NAME<void, PAWN_HOOK_TYPES>;

			subhook::Hook &
				hook_;

			implementation_t const
				original_;

			bool
				removed_;
		};

		inline void operator()(PAWN_HOOK_PARAMETERS)
		{
			if (Recursing())
			{
				subhook::ScopedHookRemove
					undo(&GetHook());
				original_(PAWN_HOOK_CALLING);
			}
			else
			{
				Do(PAWN_HOOK_CALLING);
			}
			Recursing();
		}

		ScopedCall operator*() const
		{
			ScopedCall
				ret(GetHook(), original_);
			return ret;
		}

	protected:
		PAWN_HOOK_NAME(char const * const name, implementation_t original, AMX_NATIVE replacement) : NativeHookBase(PAWN_HOOK_NUMBER, name, replacement), original_(original) {}
		~PAWN_HOOK_NAME() = default;

	private:
		cell CallDoInner(AMX * amx, cell * params)
		{
				int idx = 1;
			#if PAWN_HOOK_NUMBER >= 1
				ParamCast<A> a(amx, params, idx);
				idx += ParamCast<A>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 2
				ParamCast<B> b(amx, params, idx);
				idx += ParamCast<B>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 3
				ParamCast<C> c(amx, params, idx);
				idx += ParamCast<C>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 4
				ParamCast<D> d(amx, params, idx);
				idx += ParamCast<D>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 5
				ParamCast<E> e(amx, params, idx);
				idx += ParamCast<E>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 6
				ParamCast<F> f(amx, params, idx);
				idx += ParamCast<F>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 7
				ParamCast<G> g(amx, params, idx);
				idx += ParamCast<G>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 8
				ParamCast<H> h(amx, params, idx);
				idx += ParamCast<H>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 9
				ParamCast<I> i(amx, params, idx);
				idx += ParamCast<I>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 10
				ParamCast<J> j(amx, params, idx);
				idx += ParamCast<J>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 11
				ParamCast<K> k(amx, params, idx);
				idx += ParamCast<K>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 12
				ParamCast<L> l(amx, params, idx);
				idx += ParamCast<L>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 13
				ParamCast<M> m(amx, params, idx);
				idx += ParamCast<M>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 14
				ParamCast<N> n(amx, params, idx);
				idx += ParamCast<N>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 15
				ParamCast<O> o(amx, params, idx);
				idx += ParamCast<O>::Size;
			#endif
			#if PAWN_HOOK_NUMBER >= 16
				ParamCast<P> p(amx, params, idx);
				idx += ParamCast<P>::Size;
			#endif
				this->Do(PAWN_HOOK_CALLING);
				return 1;
		}

		virtual void Do(PAWN_HOOK_PARAMETERS) const = 0;

		implementation_t const
			original_;
	};

	template <typename RET, PAWN_HOOK_TEMPLATE>
	class NativeHook<RET(PAWN_HOOK_TYPES)> : public PAWN_HOOK_NAME<RET, PAWN_HOOK_TYPES> { protected: NativeHook(char const * const name, typename PAWN_HOOK_NAME<RET, PAWN_HOOK_TYPES>::implementation_t original, AMX_NATIVE replacement) : PAWN_HOOK_NAME<RET, PAWN_HOOK_TYPES>(name, original, replacement) {} };
};

