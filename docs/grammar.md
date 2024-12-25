$$
\begin{align}
[\text{Prog}] &\to [\text{Stmt}]^* \\ % Program

[\text{Stmt}] &\to \begin{cases}
    \text{exit}([\text{Expr}]); \\ % Exit
    \text{let ident} = [\text{Expr}]; \\ % Let
    \{[\text{Stmt}]^*\} \\ % Scope
\end{cases} \\

[\text{Expr}] &\to
\begin{cases}
    [\text{Term}] \\
    [\text{BinExpr}] \\
\end{cases}\\

[\text{BinExpr}] &\to
\begin{cases}
    [\text{Expr}] / [\text{Expr}] & \text{prec} = 2 \\
    [\text{Expr}] * [\text{Expr}] & \text{prec} = 2 \\
    [\text{Expr}] + [\text{Expr}] & \text{prec} = 1 \\
    [\text{Expr}] - [\text{Expr}] & \text{prec} = 1 \\
\end{cases}\\

[\text{Term}] &\to
\begin{cases}
    \text{int\_lit} \\
    \text{ident} \\
    ([\text{Expr}]) \\
    
\end{cases}

\end{align}
$$