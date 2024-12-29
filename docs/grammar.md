$$
\begin{align}
[\text{Prog}] &\to [\text{Stmt}]^* \\ % Program

[\text{Stmt}] &\to \begin{cases}
    \text{exit}([\text{Expr}]); \\ % Exit
    \text{print}([\text{Arg}]); \\ % Print
    \text{let ident} = [\text{Expr}]; \\ % Let
    \text{let mut ident} = [\text{Expr}]; \\ % LetMut
    \text{ident = [\text{Expr}];} \\ % Assign
    [\text{If}] \\ % If
    [\text{Scope}] \\ % Scope
\end{cases} \\

\text{[Scope]} &\to \{[\text{Stmt}]^*\} \\ % Scope

[\text{Arg}] &\to \begin{cases}
    [\text{Expr}] \\
    [\text{String}] \\
\end{cases} \\

[\text{If}] &\to \begin{cases}
    \text{if} ([\text{Expr}]) [\text{Scope}] [\text{Elif}]^* [\text{Else}]? \\
\end{cases} \\

[\text{Elif}] &\to \begin{cases}
    \text{elif} ([\text{Expr}]) [\text{Scope}] \\
\end{cases} \\

[\text{Else}] &\to \begin{cases}
    \text{else} [\text{Scope}] \\
\end{cases} \\

[\text{Expr}] &\to \begin{cases}
    [\text{Term}] \\
    [\text{BinExpr}] \\
\end{cases} \\

[\text{BinExpr}] &\to \begin{cases}
    [\text{Expr}] / [\text{Expr}] & \text{prec = 2} \\
    [\text{Expr}] * [\text{Expr}] & \text{prec = 2} \\
    [\text{Expr}] + [\text{Expr}] & \text{prec = 1} \\
    [\text{Expr}] - [\text{Expr}] & \text{prec = 1} \\
\end{cases} \\

[\text{Term}] &\to \begin{cases}
    \text{int\_lit} \\
    \text{ident} \\
    ([\text{Expr}]) \\
\end{cases} \\

[\text{String}] &\to \text{"string\_lit"} \\ % String

\end{align}
$$